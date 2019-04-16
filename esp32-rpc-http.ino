#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "time.h"

/**************************** DEBUG *******************************/
#define DEBUG true

#if DEBUG
#define DEBUG_PRINTLN(m) Serial.println(m)
#define DEBUG_PRINT(m) Serial.print(m)

#define DEBUG_PRINTLNC(m) Serial.println("[Core " + String(xPortGetCoreID()) + "]" + m)
#define DEBUG_PRINTC(m) Serial.print("[Core " + String(xPortGetCoreID()) + "]" + m)

#else
#define DEBUG_PRINTLN(m)
#define DEBUG_PRINT(m)

#define DEBUG_PRINTLNC(m)
#define DEBUG_PRINTC(m)
#endif

#define LED 2
const char* ntpServer = "a.st1.ntp.br";
const long  gmtOffset_sec = -3 * 3600;
const int   daylightOffset_sec = 0;
int maxLogSize = 20;

AsyncWebServer server(8080);
Preferences preferences;

void setupNtp() {
  DEBUG_PRINTLNC("[NTP] Setup");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  DEBUG_PRINTC("[NTP] Now: ");
  DEBUG_PRINTLN(getTimestamp());
}

String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    DEBUG_PRINTLNC("[NTP] Failed to obtain time");
    return "";
  }

  time_t now;
  time(&now);

  return String(now);
}

void setupWebServer() {
  DEBUG_PRINTLNC("[WS] Setup");

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");
  startHandlers();
  server.begin();

  DEBUG_PRINTLNC("[WS] begin");
}

void startHandlers() {
  server.on("/rpc", HTTP_POST, [](AsyncWebServerRequest * request) {
  }, NULL, [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {

    String result = processRPC((const char*)data);

    request->send(200, "application/json", result);
  });

  server.onNotFound([](AsyncWebServerRequest * request) {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      DEBUG_PRINTLNC("[WS] NOT_FOUND: ");
      request->send(404);
    }
  });
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin();
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    DEBUG_PRINTLNC("WiFi Failed!");
    return;
  }

  setupNtp();

  preferences.begin("logs", false);
  preferences.clear();
  preferences.end();

  DEBUG_PRINTC("IP Address: ");
  DEBUG_PRINTLN(WiFi.localIP());

  setupWebServer();
}

void loop() {
}
