// This source file contains the actual code for the Bluetooth module program.
// HC-05 Bluetooth Module

#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <SoftwareSerial.h>
#include <RTClib.h>

// --- Create Bluetooth serial object ---
SoftwareSerial BTSerial(2, 3); // RX, TX

// --- Define Pins ---
#define RELAY_PIN 6
#define ledGreen  9

// --- External Variables ---
extern int moisturePercent;
extern float airTemp;
extern float airHumidity;
extern float waterTemperature;
extern bool triggerAlarm;
extern RTC_DS3231 rtc;

void initBluetooth() {
  BTSerial.begin(9600);
  Serial.println("✅ Bluetooth started: Connect to 'SmartPlantMonitor'");
  BTSerial.print("Bluetooth Ready!");
}

void sendBluetoothData() {
  DateTime now = rtc.now();

  BTSerial.println("=== Plant Status ===");
  BTSerial.print("Date: ");
  BTSerial.print(now.timestamp(DateTime::TIMESTAMP_DATE));
  BTSerial.print(" | Timestamp: ");
  BTSerial.print(now.timestamp(DateTime::TIMESTAMP_TIME));

  BTSerial.print(" | Moisture: ");
  BTSerial.print(moisturePercent); BTSerial.print("%");

  BTSerial.print(" | Air Temp: ");
  BTSerial.print(airTemp, 1); BTSerial.print("°C");

  BTSerial.print(" | Air Humidity: ");
  BTSerial.print(airHumidity, 1); BTSerial.print("%");

  BTSerial.print(" | Water Temp: ");
  BTSerial.print(waterTemperature, 1); BTSerial.println("°C");

  BTSerial.println("====================");
}

// --- Manual Watering Command ---
void handleBluetooth() {
  if (BTSerial.available()) {
    char cmd = BTSerial.read();
    if (cmd == 'W' || cmd == 'w') {
      BTSerial.println("Manual watering...");
      digitalWrite(RELAY_PIN, HIGH);
      digitalWrite(ledGreen, HIGH);
    } else if (cmd == 'S' || cmd == 's') {
      BTSerial.println("Watering complete");
      digitalWrite(RELAY_PIN, LOW);
      digitalWrite(ledGreen, LOW);
    } else {
      BTSerial.print("Unknown command");
    }
  }
}

#endif
