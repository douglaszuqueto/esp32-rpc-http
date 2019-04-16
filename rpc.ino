String processRPC(String data) {
  const size_t capacity = 2 * JSON_OBJECT_SIZE(2) + 2048;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, data);

  String method = doc["method"];
  String result;

  if (method.equals("Wifi.Info")) {
    wifiInfo(&result);
  }

  if (method.equals("Wifi.SetCredentials")) {
    wifiSetCredentials(doc["params"]);
  }

  if (method.equals("ESP.Info")) {
    result = espInfo();
  }

  if (method.equals("ESP.Reboot")) {
    reboot();
  }

  if (method.equals("Led.Write")) {
    result = ledWrite(doc["params"]);
  }

  if (method.equals("Led.Toggle")) {
    result = ledToggle();
  }

  if (method.equals("Led.State")) {
    result = ledState();
  }

  if (method.equals("OTA.Update")) {
    result = otaProcess("OTA.Update");
  }

  if (method.equals("OTA.Commit")) {
    result = otaProcess("OTA.Commit");
  }

  if (method.equals("OTA.Rollback")) {
    result = otaProcess("OTA.Rollback");
  }

  if (method.equals("Config.Set")) {
    result = configSet(method, doc["params"]);
  }

  if (method.equals("Config.Get")) {
    result = configGet(method, doc["params"]);
  }

  if (method.equals("Log.All")) {
    result = logAll(method);
  }

  if (method.equals("Log.Get")) {
    result = logGet(method, doc["params"]);
  }

  writeLog(method, getTimestamp());

  return result;
}

String espInfo() {
  String result;
  const size_t capacity = JSON_OBJECT_SIZE(9) + 130;

  DynamicJsonDocument doc(capacity);
  JsonObject data = doc.createNestedObject("data");

  int sketch_size = ESP.getSketchSize();
  int flash_size =  ESP.getFreeSketchSpace();
  int available_size = flash_size - sketch_size;

  doc["method"] = "ESP.Info";
  data["chipid"] = getChipID();
  data["fw_version"] = "1.2.0";
  data["sketch_size"] = sketch_size;
  data["flash_size"] = flash_size;
  data["available_size"] = available_size;
  data["ms"] = millis();
  data["uptime"] = esp_timer_get_time();
  data["ntp"] = getTimestamp();
  data["heap_size"] = getHeapSize();
  data["free_heap"] = getFreeHeap();

  serializeJson(doc, result);
  return result;
}

void wifiInfo(String *result) {
  const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + 50;

  DynamicJsonDocument doc(capacity);
  JsonObject data = doc.createNestedObject("data");

  doc["method"] = "Wifi.Info";
  data["mode"] = int(WiFi.getMode());
  data["ssid"] = WiFi.SSID();
  data["ip"] = WiFi.localIP().toString();
  data["mac"] = String(WiFi.macAddress());

  serializeJson(doc, *result);
}

void wifiSetCredentials(JsonVariant params) {
  const char* ssid = params["ssid"];
  const char* password = params["password"];

  DEBUG_PRINTLNC("SSID " + ssid + " | Password " + password);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  reboot();
}

String ledState() {
  String result;
  const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 40;

  DynamicJsonDocument doc(capacity);
  JsonObject data = doc.createNestedObject("data");

  doc["method"] = "Led.State";
  data["status"] = digitalRead(LED);

  serializeJson(doc, result);
  return result;
}

String ledWrite(JsonVariant params) {
  int state = params["state"];
  digitalWrite(LED, state);

  String result;
  const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 40;

  DynamicJsonDocument doc(capacity);
  JsonObject data = doc.createNestedObject("data");

  doc["method"] = "Led.Write";
  data["status"] = digitalRead(LED);

  serializeJson(doc, result);
  return result;
}

String ledToggle() {
  digitalWrite(LED, !digitalRead(LED));

  String result;
  const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 40;

  DynamicJsonDocument doc(capacity);
  JsonObject data = doc.createNestedObject("data");

  doc["method"] = "Led.Toggle";
  data["status"] = digitalRead(LED);

  serializeJson(doc, result);
  return result;
}

String otaProcess(String method) {
  String result;
  const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 40;

  DynamicJsonDocument doc(capacity);
  JsonObject data = doc.createNestedObject("data");

  doc["method"] = method;
  data["state"] = "not implemented";

  serializeJson(doc, result);
  return result;
}

String configGet(String method, JsonVariant params) {
  String key = params["key"];

  preferences.begin("config", true);
  String value = preferences.getString(key.c_str());
  preferences.end();

  String result;
  const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 40;

  DynamicJsonDocument doc(capacity);
  JsonObject data = doc.createNestedObject("data");

  doc["method"] = method;
  data[key] = value;

  serializeJson(doc, result);
  return result;
}

String configSet(String method, JsonVariant params) {
  String key = params["key"];
  String value = params["value"];

  preferences.begin("config", false);
  preferences.putString( key.c_str(), value);
  preferences.end();

  String result;
  const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 40;

  DynamicJsonDocument doc(capacity);
  JsonObject data = doc.createNestedObject("data");

  doc["method"] = method;
  data[key] = value;

  serializeJson(doc, result);
  return result;
}

String logAll(String method) {
  String result;
  const size_t capacity = JSON_ARRAY_SIZE((maxLogSize + 10)) + 2048;

  DynamicJsonDocument doc(capacity);
  JsonArray data = doc.createNestedArray("data");

  doc["method"] = method;

  preferences.begin("logs", false);
  for (int i = 0; i < maxLogSize; i++ ) {
    String idx = "log_" + String(i);
    String item = preferences.getString(idx.c_str());

    data.add(item);
  }
  preferences.end();
  serializeJson(doc, result);

  return result;
}

String logGet(String method, JsonVariant params) {
  String key = params["key"];
  String result;

  const size_t capacity = JSON_ARRAY_SIZE((maxLogSize + 10)) + 2048;

  DynamicJsonDocument doc(capacity);
  JsonArray data = doc.createNestedArray("data");

  preferences.begin("logs", false);
  String idx = "log_" + key;
  String item = preferences.getString(idx.c_str());
  preferences.end();

  doc["method"] = method;
  data.add(item);
  serializeJson(doc, result);

  return result;
}

// #############################

String getFreeHeap() {
  return String(ESP.getFreeHeap());
}

String getHeapSize() {
  return String(ESP.getHeapSize());
}

// https://github.com/lucafabbri/HiGrow-Arduino-Esp/blob/master/HiGrowEsp32/HiGrowEsp32.ino
String getChipID() {
  char id[15];
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(id, "ESP32-%08X", (uint32_t)chipid);

  return String(id);
}

void reboot() {
  DEBUG_PRINTLNC("[ESP] rebooting");
  esp_restart();
}

void resetLogCounter() {
  DEBUG_PRINTLNC("[Log] set counter to 0");
  preferences.putInt("logs", 0);
}

void incrementLogCounter() {
  int logs = preferences.getInt("logs");
  preferences.putInt("logs", (logs + 1));
}

void checkLogSize() {
  int logs = preferences.getInt("logs");

  if (logs >= maxLogSize) {
    resetLogCounter();
  }
}

void writeLog(String data, String timestamp) {
  preferences.begin("logs", false);
  String result;
  StaticJsonDocument<500> root;

  root["method"] = data;
  root["created_at"] = timestamp;
  serializeJson(root, result);

  checkLogSize();

  int logs = preferences.getInt("logs");
  String idx = "log_" + String(logs);
  preferences.putString(idx.c_str(), result);

  incrementLogCounter();
  result = String();
  preferences.end();
  DEBUG_PRINTLNC("[Log] Saved: " + timestamp);
}
