/***************************************************************************
Author: Kewin Wan, Chen Yu Wei
Subject: Team Project
Title: Smart Plant Monitoring System
Programme: Mechanical and Manufacturing Engineering
Institution: Munster Technological University

Hardware: Arduino Uno R4 WiFi, HC-05, Micro SD Card Adapter,
          Capacitive Soil Moisture Sensor v2.0, DHT11, DS18B20,
          LCD1602I2C, 5V Relay, Water Pump, PiezoBuzzer, LEDs.
Software: Arduino IDE, ThingSpeak server
***************************************************************************/
// This source file is the main program of the Smart Plant Monitoring System, all the sub-functions are called and run in this program.

// --- Libraries ---
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <Wire.h>

#include "Bluetooth.h"
#include "Droplet.h"
#include "Logger.h"
#include "Sensors.h"
#include "WiFiThingSpeak.h"

// --- Define Pins ---
#define SOIL_PIN    A0
#define RELAY_PIN   6
#define BUZZER_PIN  7
#define ledRed      8
#define ledGreen    9

// --- LCD & RTC setup ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

// --- Soil Calibration Values ---  (Adjust based on your reference)
int dryValue = 540;
int wetValue = 265;
int moisturePercent;
const int moistureThreshold = 20; // Moisture below 20%
bool moistErrorPrinted = false;

// --- Timing Variables ---
unsigned long lastPageSw = 0;
int currentPage = 0;
const int pageDelay = 3000; // Sw page every 3 secs

unsigned long lastUpdateTime = 0;
unsigned long lastWateringTime = 0;
bool watering = false;

bool triggerAlarm = false;
bool buzzerState = false; // true = ON, false = OFF
unsigned long lastBeepTime = 0;
const unsigned long beepInterval = 500;

bool airTempAlertPrinted = false;
bool airHumAlertPrinted = false;
bool waterTempAlertPrinted = false;

void setup() {
  Serial.begin(9600);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Ensure pump is off at start
  noTone(BUZZER_PIN);            // Ensure buzzer off at start
  digitalWrite(ledRed, LOW);
  digitalWrite(ledGreen, LOW);

  // --- Sub-Functions init ---
  initBluetooth();
  createDroplet(lcd);
  initLogger();
  initSensors();
  initWiFi();
  
  // --- LCD init ---
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); lcd.print("Smart Plant");
  lcd.setCursor(0,1); lcd.print("System Ready!");

  // --- RTC init ---
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set from compile time
  }

  Serial.println("Smart Plant Watering System Ready");
  Serial.println("==============================");
  
  delay(2000); // Pre-start message display
  lcd.clear();
}

void loop() {

  // --- Call out Sub-Functions ---
  handleBluetooth();  // Check for Bluetooth commands
  if (millis() % 5000 < 50) sendBluetoothData(); // Bluetooth data update every 5s
  handleLogging();    // Log data to SD card every 30 secs
  bool sensorOk = readSensors(); // Read and check all sensors
  handleThingSpeak(); // Online data update every 20s

  unsigned long currentTime = millis();

  // --- Read Soil Moisture ---
  int soilValue = analogRead(SOIL_PIN);
  moisturePercent = map(soilValue, dryValue, wetValue, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);
  if (soilValue <= 10 && soilValue >= 1020) {
    Serial.println("Moisture sensor not detected or faulty!");
    moistErrorPrinted = true;
  }
  if (soilValue > 10 && soilValue < 1020) {
    moistErrorPrinted = false;
  }
  
  // --- Sensors Serial Monitor Display Update (every 2 sec) ---
  if (sensorOk && !triggerAlarm) {
    if (currentTime - lastUpdateTime >= 2000) {
      lastUpdateTime = currentTime;
      Serial.print("Moisture: ");
      Serial.print(moisturePercent); Serial.print("% | "); Serial.println(soilValue);
  
      Serial.print("Air Temp: ");
      Serial.print(airTemp); Serial.println("°C");
      
      Serial.print("Air Humidity: ");
      Serial.print(airHumidity); Serial.println("%");
  
      Serial.print("Water Temp: ");
      Serial.print(waterTemperature); Serial.println("°C");
      Serial.println("==============================");
    }
  }

  // --- Read RTC Time ---
  DateTime now = rtc.now();
  char dateBuffer[11];
  char timeBuffer[9];
  sprintf(dateBuffer, "%02d/%02d/%04d", now.day(), now.month(), now.year());
  sprintf(timeBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  // --- Display Sensor Data (cycling pages) ---
  if (!watering && (currentTime - lastPageSw >= pageDelay)) {
    lastPageSw = currentTime;
    currentPage = (currentPage + 1) % 3;
    lcd.clear();

    // --- IF NOT sensors failed THEN print the values ---
    if (!sensorOk) {
      lcd.setCursor(0,0); lcd.print("Sensor Error!");
      lcd.setCursor(0,1); lcd.print("Check Sensors!");
      
    } else {
      if (currentPage == 0) {
        // --- Date & Time Display ---
        lcd.setCursor(0,0);
        lcd.print("Date: "); lcd.print(dateBuffer);
        lcd.setCursor(0,1);
        lcd.print("Time: "); lcd.print(timeBuffer);
      }
      if (currentPage == 1) {
        // --- Moisture & Water Temp display ---
        lcd.setCursor(0,0);
        lcd.print("Moist: ");
        lcd.print(moisturePercent); lcd.print("%");

        lcd.setCursor(0,1);
        lcd.print("Water: ");
        lcd.print(waterTemperature, 1); lcd.print(char(223)); lcd.print("C");
        
      } else if (currentPage == 2) {
        // --- Air Temp & Air Humidity display ---
        lcd.setCursor(0,0);
        lcd.print("Air Temp: ");
        lcd.print(airTemp, 1); lcd.print(char(223)); lcd.print("C");
        
        lcd.setCursor(0,1);
        lcd.print("Humidity: ");
        lcd.print(airHumidity, 1); lcd.print("%");
      }
    }
  }
  
  // --- Auto Watering Logic ---
  if (sensorOk && !triggerAlarm) {
    if (!watering && moisturePercent < 20 && (now.hour() >= 6 && now.hour() <= 20)) {
      Serial.println("\nWatering plant...");
      digitalWrite(RELAY_PIN, HIGH);
      digitalWrite(ledGreen, HIGH);
      watering = true;
      lastWateringTime = currentTime;
  
      lcd.clear();
      lcd.setCursor(3,0);
      lcd.print("Watering...");
    }
  }

  // --- Watering Animation ---
  if (watering) {
    static int animationPosn = 0;
    lcd.setCursor(animationPosn,1); // Move the droplet across second line
    lcd.write(byte(0)); // Droplet saved as '0'
    int previousPosn = animationPosn -1; // Clear previous droplet
    if (previousPosn < 0) previousPosn = 15; // Wrap around LCD
    lcd.setCursor(previousPosn,1);
    lcd.print(" ");

    animationPosn++;
    if (animationPosn > 15) animationPosn = 0;
    //delay(100); //(adjust for slower/ faster animation)
  }

  // --- Stop Watering after 10 secs ---
  if (watering && (currentTime - lastWateringTime >= 10000)) {
    Serial.println("\nWatering complete!");
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(ledGreen, LOW);
    watering = false;
    lcd.clear();
  }

  // --- Check for Alarm System ---
  checkAlarms();
  if (triggerAlarm) {
    if (currentTime - lastBeepTime >= beepInterval) {
      lastBeepTime = currentTime;
      buzzerState = !buzzerState;

      // Non-Blocking Alarm Alerts
      if (buzzerState) {
        tone(BUZZER_PIN, 500);
        digitalWrite(ledRed, HIGH);
      } else {
        noTone(BUZZER_PIN);
        digitalWrite(ledRed, LOW);
      }
    }
  } else {
    noTone(BUZZER_PIN);
    digitalWrite(ledRed, LOW);
    buzzerState = false;
  }
}

void checkAlarms() {
  triggerAlarm = false;

  if (waterTemperature < 5 || waterTemperature > 35) {
    triggerAlarm = true;
    if (!waterTempAlertPrinted) {
      Serial.println("Water temp out of range!");
      BTSerial.println("Water temp out of range!");
      waterTempAlertPrinted = true;
    }
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Water Temp");
    lcd.setCursor(0,1); lcd.print("Out of Range!");
  }
  
  if (airTemp > 35) {
    triggerAlarm = true;
    if (!airTempAlertPrinted) {
      Serial.println("Air temp too high!");
      BTSerial.println("Air temp too high!");
      airTempAlertPrinted = true;
    }
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Air Temp");
    lcd.setCursor(0,1); lcd.print("too High!");
  }
  
  if (airHumidity < 20) {
    triggerAlarm = true;
    if (!airHumAlertPrinted) {
      Serial.println("Humidity too low!");
      BTSerial.println("Humidity too low!");
      airHumAlertPrinted = true;
    }
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Air Humidity");
    lcd.setCursor(0,1); lcd.print("too Low!");
  }

  // Prevent alert message from flooding the monitor
  if (waterTemperature >= 5 && waterTemperature <= 35) waterTempAlertPrinted = false;
  if (airTemp <= 35) airTempAlertPrinted = false;
  if (airHumidity >= 20) airHumAlertPrinted = false;
}
