#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include "esp_camera.h"
#include <Arduino.h>
#include "FS.h"
#include "SPI.h"
#include "SdFat.h"
using namespace websockets;

// Camera pins for ESP32-CAM
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

WebsocketsClient client; // Declare only once
bool client_connected = false;

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

  // Initialize with high specs to pre-allocate larger buffers
  config.frame_size = FRAMESIZE_QVGA; // 320x240
  config.jpeg_quality = 10;           // 0-63 lower number means higher quality
  config.fb_count = 1;

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // Initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID)
  {
    s->set_vflip(s, 1);       // flip it back
    s->set_brightness(s, 1);  // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
}

void onMessageCallback(WebsocketsMessage message)
{
  // Handle incoming messages if needed
  Serial.print("Got Message: ");
  Serial.println(message.data());
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
  client.connect(websocket_server, websocket_port, "/ws");

  // Send a ping every 15000 ms (15 seconds)
  // client.setPingInterval(15000);
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
    client.send("ping"); // Send a custom ping message
    lastPingTime = millis();
    Serial.println("Ping sent");
  }

  if (client_connected)
  {
    captureAndSendFrame();
    delay(100); // Adjust frame rate by changing delay
  }
  else
  {
    // Try to reconnect to WebSocket server
    if (!client.connect(websocket_server, websocket_port, "/ws"))
    {
      Serial.println("WebSocket connection failed. Retrying in 5 seconds...");
      delay(5000);
    }
  }
}