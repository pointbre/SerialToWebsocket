/*
 * Serial to Websocket converter
 * - Read a line from serial and then broadcast it to Websocket
 * - For RFLink gateway packet
 * - This is to add mDNS, OTA update and a few things to the following original source:
 *   https://github.com/tzapu/WebSocketSerialMonitor/blob/master/WebSocketSerialMonitor.ino
 * */
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>   // https://github.com/Links2004/arduinoWebSockets
#include <Hash.h>
#include <DNSServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
WebSocketsServer webSocket = WebSocketsServer(81);
String inputString = "";

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
  switch (type) {
    case WStype_DISCONNECTED:
      break;
    case WStype_CONNECTED:
      webSocket.sendTXT(num, "Connected to Serial To Websocket converter\n");
      break;
    case WStype_TEXT:        
      Serial.printf("%s\n", payload);
      break;
    case WStype_BIN:
      webSocket.sendTXT(num, "Binary transmission is not supported");
      break;
  }
}

void setup() {
  Serial.begin(57600);  
  
  WiFiManager wifiManager;
  if (!wifiManager.autoConnect("YOUR_WIFI_AP", "YOUR_WIFI_PW")) {
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // mDNS
  // OTA update - http://address_you_want.local:80/update
  // Websocket - ws://address_you_want.local:81
  if (!MDNS.begin("address_you_want")) {
    while (1) {
      delay(500);
    }
  }
  MDNS.addService("http", "tcp", 80); 
  MDNS.addService("ws", "tcp", 81); 

  inputString.reserve(256);

  httpUpdater.setup(&httpServer);
  httpServer.begin();
}

void serialEvent() {
  inputString = "";
  if (Serial.available()) {
    inputString = Serial.readStringUntil('\n');
  }
}

void loop() {
  httpServer.handleClient(); 
  webSocket.loop();  
  serialEvent();
  if (inputString.length() > 0) {
    webSocket.broadcastTXT(inputString);
  }
}
