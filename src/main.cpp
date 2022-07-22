#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#include <FS.h>
#include <LittleFS.h>
#include <Filemanager.h>

//Variables to save values from HTML form
String ssid = "LTest";
String pass = "ChangeMe";
const char* http_username = "admin";
const char* http_password = "admin";

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);


// Initialize SPIFFS
void initFS() {
  if (!LittleFS.begin(false,"")) {
    Serial.println("LITTLEFS Mount fehlgeschlagen");
    Serial.println("Kein Dateisystemsystem gefunden; wird formatiert");
    // falls es nicht klappt, erneut mit Neu-Formatierung versuchen
    if (!LittleFS.begin(true,"")) {
      Serial.println("LITTLEFS Mount fehlgeschlagen");
      Serial.println("Formatierung nicht möglich");
      return;
    } else {
      Serial.println("Formatierung des Dateisystems erfolgt");
    }
  }
}

// Initialize WiFi
bool initWiFi() {
  if(ssid==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");
  unsigned long currentMillis = millis();
  previousMillis = currentMillis;
  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }
  Serial.println(WiFi.localIP());
  return true;
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  initFS();
  initWiFi();


  WiFi.scanNetworks(true);
  server.addHandler(new Filemanager(LittleFS, http_username,http_password));
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });
  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest * request) {
    String json = "[";
    int n = WiFi.scanComplete();
    if (n == -2) {
      WiFi.scanNetworks(true);
    } else if (n) {
      for (int i = 0; i < n; ++i) {
        if (i) json += ",";
        json += "{";
        json += "\"ssid\":" + WiFi.SSID(i);
        json += ",\"rssi\":\"" + String(WiFi.RSSI(i)) + "\"";
        json += ",\"channel\":" + String(WiFi.channel(i));
        json += "}";
      }
      WiFi.scanDelete();
      if (WiFi.scanComplete() == -2) {
        WiFi.scanNetworks(true);
      }
    }
    json += "]";
    request->send(200, "application/json", json);
    json = String();
  });
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
}