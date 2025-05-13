//This source file contains the actual code for the SD card data logging program.
// Micro SD Card Adapter Module

#ifndef LOGGER_H
#define LOGGER_H

#include <SD.h>
#include <SPI.h>
#include <RTClib.h>

// ---SD Card Settings ---
#define SD_CS_PIN 10
#define SOIL_PIN  A0
File dataFile;

// --- Logging Time Control ---
unsigned long lastLogTime = 0;
const unsigned long logInterval = 10000;  // 10 secs logging interval
bool debugLogging = true;

// --- External Sensor Data Preferences ---
extern int dryValue;
extern int wetValue;
extern float airTemp;
extern float airHumidity;
extern float waterTemperature;
extern RTC_DS3231 rtc;

void initLogger() {
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card initialization failed!");
    while (1); // Stop if SD card fails
  } else {
    if (debugLogging) Serial.println("SD Card initializes.");
  }
}

// --- Get File Name from Date ---
String getLogFileName(const DateTime& now) {
  char fileName[17];
  sprintf(fileName, "%04d%02d%02d.csv", now.year(), now.month(), now.day());
  return String(fileName);
}

// --- Log Data to SD Card ---
void logData() {
  DateTime now = rtc.now();
  if (!now.isValid()) {
    if (debugLogging) Serial.println("RTC time invalid! - skipping log.");
  }

  String fileName = getLogFileName(now);
  bool newFile = !SD.exists(fileName); // Check if the file is already exist

  dataFile = SD.open(fileName, FILE_WRITE);
  if (dataFile) {
    if (newFile) {
      dataFile.println("Timestamp,Moisture(%),AirTemp(C),Humidity(%),WaterTemp(C)");
    }

    // --- Read and calculate data ---
    int soilValue = analogRead(SOIL_PIN);
    int moisturePercent = map(soilValue, dryValue, wetValue, 0, 100);
    moisturePercent = constrain(moisturePercent, 0, 100);

    char timestamp[9];
    sprintf(timestamp, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    // --- Log Data ---
    dataFile.print(timestamp); dataFile.print(",");
    dataFile.print(moisturePercent); dataFile.print(",");
    dataFile.print(airTemp, 1); dataFile.print(",");
    dataFile.print(airHumidity, 1); dataFile.print(",");
    dataFile.print(waterTemperature, 1);
    dataFile.close(); // Save and close

    if (debugLogging) {
      Serial.print("Data logged to ");
      Serial.println(fileName);
    } else {
      if (debugLogging) Serial.println("Failed to open log file.");
    }
  }
}

// --- Log Sensors Data (every 10 secs) ---
void handleLogging() {
  unsigned long currentTime = millis();
  if (currentTime - lastLogTime >= logInterval) {
    lastLogTime = currentTime;
    logData();
  }
}

#endif
