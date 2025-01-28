#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <IRremote.h>

using namespace websockets;

// WiFi credentials
const char *ssid = "ybc2";
const char *password = "20010219";

// WebSocket server details
const char *websocket_server = "ws://121.43.141.184:8080/remote";

// IR LED pin
const int IR_LED_PIN = 15;

// IR codes
const uint32_t IR_VOICE_PLUS = 0x44BB01FE;
const uint32_t IR_VOICE_MINUS = 0x44BB817E;
const uint32_t IR_UP = 0x44BB53AC;
const uint32_t IR_LEFT = 0x44BB9966;
const uint32_t IR_RIGHT = 0x44BB837C;
const uint32_t IR_DOWN = 0x44BB4BB4;
const uint32_t IR_OK = 0x44BB738C;
const uint32_t IR_MENU = 0x44BB11EE;
const uint32_t IR_BACK = 0x44BBA956;

WebsocketsClient client;

void sendIRCode(uint32_t code)
{
  IrSender.sendNEC(code, 32);
  Serial.printf("Sent IR code: 0x%08X\n", code);
  delay(100); // Small delay between transmissions
}

void processCommand(String command)
{
  if (command == "voice+" || command == "power")
    sendIRCode(IR_VOICE_PLUS);
  else if (command == "voice-" || command == "temp_up")
    sendIRCode(IR_VOICE_MINUS);
  else if (command == "up" || command == "temp_down")
    sendIRCode(IR_UP);
  else if (command == "left" || command == "mode")
    sendIRCode(IR_LEFT);
  else if (command == "right" || command == "fan")
    sendIRCode(IR_RIGHT);
  else if (command == "down" || command == "timer")
    sendIRCode(IR_DOWN);
  else if (command == "ok" || command == "swing")
    sendIRCode(IR_OK);
  else if (command == "manu" || command == "menu" || command == "sleep")
    sendIRCode(IR_MENU);
  else if (command == "back" || command == "eco")
    sendIRCode(IR_BACK);
}

void onMessageCallback(WebsocketsMessage message)
{
  String command = message.data();
  Serial.printf("Got Message: %s\n", command.c_str());
  processCommand(command);
}

void onEventsCallback(WebsocketsEvent event, String data)
{
  if (event == WebsocketsEvent::ConnectionOpened)
  {
    Serial.println("WebSocket Connection Opened");
  }
  else if (event == WebsocketsEvent::ConnectionClosed)
  {
    Serial.println("WebSocket Connection Closed");
  }
}

void setup()
{
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Initialize IR sender
  IrSender.begin(IR_LED_PIN);

  // Setup Callbacks
  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);

  // Connect to WebSocket server
  bool connected = client.connect(websocket_server);
  if (connected)
  {
    Serial.println("Connected to WebSocket server!");
  }
  else
  {
    Serial.println("Failed to connect to WebSocket server!");
  }
}

void loop()
{
  if (client.available())
  {
    client.poll();
  }
  else
  {
    Serial.println("WebSocket connection lost, reconnecting...");
    client.connect(websocket_server);
    delay(5000);
  }
}