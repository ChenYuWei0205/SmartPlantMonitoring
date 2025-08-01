// This source file contains the actual code for the online data logging program integrate with ThingSpeak server.
// Arduino Uno R4 WiFi, ThingSpeak Server
// Take note that ThingSpeak free access only allow data upload every 15 secs (faster uploading requires a premium account)

#ifndef WIFITHINGSPEAK_H
#define WIFITHINGSPEAK_H

#include <WiFiS3.h>
#include <ThingSpeak.h>
#include <Arduino.h>

// --- Wi-Fi Credentials (replace with your own) ---
char ssid[] = "Almondddd";             // <-- Your WiFi network name
char pass[] = "WelcomeHello";         // <-- Your WiFi password

// --- ThingSpeak ---
unsigned long channelID = 2951105;           // Replace with your ThingSpeak channel number
const char *apiKey = "DFE3FGGUPOUK8Y8T";     // Replace with your ThingSpeak Write API Key

WiFiClient client;

// --- External Sensor Variables ---
extern float airTemp;
extern float airHumidity;
extern float waterTemperature;
extern int moisturePercent;

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 20000;  // 20 seconds

void initWiFi() {
  Serial.print("Connecting to WiFi");

  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client);
}

void uploadToThingSpeak() {

  // --- Write Fields ---
  ThingSpeak.setField(1, moisturePercent);
  ThingSpeak.setField(2, airTemp);
  ThingSpeak.setField(3, airHumidity);
  ThingSpeak.setField(4, waterTemperature);

  int response = ThingSpeak.writeFields(channelID, apiKey);

  if (response == 200) {
    Serial.println("Data sent to ThingSpeak successfully.");
  } else {
    Serial.print("ThingSpeak error: ");
    Serial.println(response);
  }
}

// --- Upload data online every 20s ---
// (ThingSpeak free access only allow data to be uploaded every 15s, set to 20s for stable data uploading.)
void handleThingSpeak() {
  unsigned long currentTime = millis();
  if (currentTime - lastUpdate >= updateInterval) {
    lastUpdate = currentTime;

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected. Reconnecting...");
      initWiFi();
    }

    uploadToThingSpeak();
  }
}

#endif
