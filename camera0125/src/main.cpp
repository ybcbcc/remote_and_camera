#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include "esp_camera.h"
#include <Arduino.h>
#include "FS.h"
#include "SPI.h"
#include "SdFat.h"
using namespace websockets;

// Camera pins for ESP32-CAM (保持原有定义不变)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// WiFi credentials
const char *ssid = "ybc2";
const char *password = "20010219";

// WebSocket server details
const char *websocket_server = "121.43.141.184";
const uint16_t websocket_port = 8080;

WebsocketsClient client;
bool client_connected = false;
bool camera_active = false; // 新增摄像头状态控制变量

void configCamera()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID)
  {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }
}

void onMessageCallback(WebsocketsMessage message)
{
  String msg = message.data();
  Serial.print("Got Message: ");
  Serial.println(msg);

  // 处理控制命令
  if (msg == "start_camera")
  {
    camera_active = true;
    Serial.println("Camera activated");
    client.send("camera_started"); // 发送确认消息
  }
  else if (msg == "stop_camera")
  {
    camera_active = false;
    Serial.println("Camera deactivated");
    client.send("camera_stopped"); // 发送确认消息
  }
}

void onEventsCallback(WebsocketsEvent event, String data)
{
  if (event == WebsocketsEvent::ConnectionOpened)
  {
    Serial.println("Connection Opened");
    client_connected = true;
  }
  else if (event == WebsocketsEvent::ConnectionClosed)
  {
    Serial.println("Connection Closed");
    client_connected = false;
    camera_active = false; // 连接断开时停止摄像
  }
  else if (event == WebsocketsEvent::GotPing)
  {
    Serial.println("Got a Ping!");
  }
  else if (event == WebsocketsEvent::GotPong)
  {
    Serial.println("Got a Pong!");
  }
}

void captureAndSendFrame()
{
  if (!camera_active)
    return; // 如果摄像头未激活，直接返回

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    return;
  }

  if (client_connected)
  {
    client.sendBinary((const char *)fb->buf, fb->len);
  }

  esp_camera_fb_return(fb);
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Initialize camera
  configCamera();

  // Configure WebSocket client
  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);

  // Connect to WebSocket server
  client.connect(websocket_server, websocket_port, "/camera");
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi connection lost. Reconnecting...");
    WiFi.begin(ssid, password);
    delay(5000);
    return;
  }

  client.poll();

  // Send custom ping message every 15 seconds
  static unsigned long lastPingTime = 0;
  if (millis() - lastPingTime > 15000)
  {
    client.send("ping");
    lastPingTime = millis();
    Serial.println("Ping sent");
  }

  if (client_connected)
  {
    captureAndSendFrame(); // 只有在camera_active为true时才会实际捕获和发送帧
    if (camera_active)
    {
      delay(100); // 只在实际发送帧时增加延迟
    }
  }
  else
  {
    if (!client.connect(websocket_server, websocket_port, "/camera"))
    {
      Serial.println("WebSocket connection failed. Retrying in 5 seconds...");
      delay(5000);
    }
  }
}
