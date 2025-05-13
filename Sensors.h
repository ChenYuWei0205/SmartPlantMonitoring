//This source file contains the actual code for the air temperature, humidity, and water temperature ADC program.
//DHT11, DS18B20 Modules

#ifndef SENSORS_H
#define SENSORS_H

#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- DHT11 Sensor Setup ---
#define DHTPIN 5       // DHT11 connected to D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- DS18B20 Sensor Setup ---
#define ONE_WIRE_BUS 4  // DS18B20 connected to D7
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature waterSensor(&oneWire);

// --- Sensors Variables ---
float airTemp = 0.0;
float airHumidity = 0.0;
float waterTemperature = 0.0;
const int airTempThreshold = 35;
const int airHumidityThreshold = 40;
const int lowWaterThreshold = 5;
const int highWaterThreshold = 35;

unsigned long lastTempReqTime = 0;
bool waterTempRequested = false;

bool dhtTempErrorPrinted = false;
bool dhtHumErrorPrinted = false;
bool dsbErrorPrinted = false;

void initSensors() {
  dht.begin();
  waterSensor.begin();
  waterSensor.setResolution(9); // Defaul resolution is 12-bits
}

bool readSensors() {
  bool success = true; // Assume everything is fine initally
  
  // --- Read Air Temperature and Humidity ---
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (!isnan(temp)) {
    airTemp = temp;
    dhtTempErrorPrinted = false;
  } else {
    success = false;
    if (!dhtTempErrorPrinted) {
      Serial.println("Failed to read DHT11 Temp");
      dhtTempErrorPrinted = true;
    }
  }

  if (!isnan(hum)) {
    airHumidity = hum;
    dhtHumErrorPrinted = false;
  } else {
    success = false;
    if (!dhtHumErrorPrinted) {
      Serial.println("Failed to read DHT11 Humidity");
      dhtHumErrorPrinted = true;
    }
  }

  // --- Read Water Temperature (Non-Blocking) --- (REMOVE COMMENT FOR ALTERNATIVE)
  unsigned long currentTime = millis();
  
  // --- Get Temp reading whenever function is called ---
  /*if (!waterTempRequested) {
    lastTempReqTime = currentTime;
    waterSensor.requestTemperatures();
    waterTempRequested = true;
  } 
  else if (currentTime - lastTempReqTime >= 100) {
    float waterTemp = waterSensor.getTempCByIndex(0);

    if (waterTemp != DEVICE_DISCONNECTED_C) {
      waterTemperature = waterTemp;
    } else {
      Serial.println("Failed to read DS18B20 Water Temperature!");
      success = false;
    }
    waterTempRequested = false; // Ready for next cycle
  }*/
  
    // --- Call Temp reading every 100ms ---
    if (currentTime - lastTempReqTime >= 100) {
    lastTempReqTime = currentTime;
    waterSensor.requestTemperatures();
    float waterTemp = waterSensor.getTempCByIndex(0);
  
    if ((waterTemp != DEVICE_DISCONNECTED_C)) {
      waterTemperature = waterTemp;
      dsbErrorPrinted = false;
    } else {
      success = false;
      if (!dsbErrorPrinted) {
        Serial.println("Failed to read DS18B20 sensor!");
        dsbErrorPrinted = true;
      }
    }
  }

  return success; // Return overall sensor status
}

#endif
