// ===========================
// OTAHelper.ino
// WiFi + OTA setup
// ===========================

#include <WiFi.h>
#include <ArduinoOTA.h>

const char* WIFI_SSID     = "B&S World 2G";
const char* WIFI_PASSWORD = "bruceandsoph2022";
const char* OTA_PASSWORD  = "password";

void setupOTA() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();
  Serial.println("OTA ready");
}

void handleOTA() {
  ArduinoOTA.handle();
}