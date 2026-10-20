/*
 * ATOMS — Automated Organ Transport Monitoring System
 * ESP32 prototype firmware 
 * DS18B20 temperature (GPIO4, 4.7k pull-up to 3.3V)
 * MPU6050 acceleration (I2C SDA21/SCL22)
 * MFRC522 RFID (SPI SS5/RST15, SCK18/MISO19/MOSI23)
 * 16x2 LCD with PCF8574 backpack (I2C, typically 0x27)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <math.h>
#include "config.h"

#define ONE_WIRE_PIN 4
#define RFID_SS_PIN 5
#define RFID_RST_PIN 15
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

const unsigned long SAMPLE_INTERVAL_MS = 5000;
const unsigned long WIFI_RETRY_INTERVAL_MS = 30000;
const unsigned long LCD_REFRESH_INTERVAL_MS = 1000;

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature tempSensor(&oneWire);
Adafruit_MPU6050 mpu;
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

String lastOrganId = "UNASSIGNED";
String lastRfidUid = "";
bool mpuAvailable = false;
bool transportActive = false;
bool remainingSynced = false;
long remainingMin = -1;
long prevTransit = -1;
unsigned long remainingSyncMs = 0;
unsigned long lastSampleMs = 0;
unsigned long lastLcdMs = 0;
unsigned long lastWifiRetryMs = 0;
float latestTemperature = NAN;
float latestVibration = NAN;

void lcdBanner(const char* top, const char* bottom) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(top);
  lcd.setCursor(0, 1);
  lcd.print(bottom);
  delay(1200);
}

void connectWiFi() {
#if ENABLE_BACKEND
  if (WiFi.status() == WL_CONNECTED) return;
  if (strlen(WIFI_SSID) == 0 || strlen(SERVER_URL) == 0) {
    Serial.println("Backend enabled but WiFi/server not configured");
    return;
  }
  Serial.println("Connecting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; ++i) {
    delay(500);
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "WiFi connected" : "WiFi unavailable; local monitoring continues");
#endif
}

void readRfid() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;
  String uid;
  for (byte i = 0; i < rfid.uid.size; ++i) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  lastRfidUid = uid;
  lastOrganId = uid;  // Prototype: UID acts as ID, not verified organ metadata
  Serial.println("RFID UID: " + uid);
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

float readVibration() {
  if (!mpuAvailable) return NAN;
  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  const float magnitude = sqrtf(a.acceleration.x * a.acceleration.x +
                                a.acceleration.y * a.acceleration.y +
                                a.acceleration.z * a.acceleration.z);
  return fabsf(magnitude - 9.81f) / 9.81f;
}

void updateLcd() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (isnan(latestTemperature)) lcd.print("T:--.-C");
  else {
    lcd.print("T:"); lcd.print(latestTemperature, 1); lcd.print("C");
  }
  lcd.print(" V:");
  if (isnan(latestVibration)) lcd.print("--");
  else lcd.print(latestVibration, 2);

  lcd.setCursor(0, 1);
  if (transportActive && remainingSynced) {
    long elapsed = (long)((millis() - remainingSyncMs) / 60000UL);
    long left = max(0L, remainingMin - elapsed);
    lcd.print("Left:"); lcd.print(left / 60);
    lcd.print("h "); lcd.print(left % 60); lcd.print("m");
  } else {
    lcd.print("Not in transit");
  }
}

#if ENABLE_BACKEND
void sendReading(float temperature, float vibration) {
  if (WiFi.status() != WL_CONNECTED) return;
  if (isnan(temperature) || isnan(vibration)) {
    Serial.println("Skipping upload: sensor reading unavailable");
    return;
  }

  StaticJsonDocument<256> doc;
  doc["organId"] = lastOrganId;
  doc["temperature"] = temperature;
  doc["vibration"] = vibration;
  doc["latitude"] = 12.9716;  // Demo placeholder, NOT live GPS
  doc["longitude"] = 77.5946; // Demo placeholder, NOT live GPS
  doc["lockStatus"] = true;   // Demo placeholder, NOT actual lock sensing
  doc["rfidUid"] = lastRfidUid;
  String payload;
  serializeJson(doc, payload);

  WiFiClientSecure client;
  // Prototype only: certificate validation disabled.
  // Replace with a trusted root CA for production use.
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(4000);
  if (!http.begin(client, SERVER_URL)) return;
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  Serial.printf("POST status: %d\n", code);
  if (code >= 200 && code < 300) {
    StaticJsonDocument<1024> response;
    if (deserializeJson(response, http.getString()) == DeserializationError::Ok) {
      long transit = response["transitMinutes"].isNull() ? -1 : response["transitMinutes"].as<long>();
      bool active = transit >= 0;
      if (active && !transportActive) lcdBanner("Transport", "STARTED");
      else if (!active && transportActive) lcdBanner("Transport", "ENDED");
      else if (active && transportActive && prevTransit >= 0 && transit < prevTransit)
        lcdBanner("Transport", "RESET");
      transportActive = active;
      prevTransit = transit;
      if (active && !response["remainingMinutes"].isNull()) {
        remainingMin = response["remainingMinutes"].as<long>();
        remainingSyncMs = millis();
        remainingSynced = true;
      } else if (!active) {
        remainingSynced = false;
      }
    }
  }
  http.end();
}
#endif

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.print("ATOMS booting...");
  tempSensor.begin();
  mpuAvailable = mpu.begin();
  if (mpuAvailable) {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  } else {
    Serial.println("MPU6050 unavailable; vibration shown as --");
  }
  SPI.begin();
  rfid.PCD_Init();
  connectWiFi();
  updateLcd();
}

void loop() {
  readRfid();
  const unsigned long now = millis();
#if ENABLE_BACKEND
  if (WiFi.status() != WL_CONNECTED && now - lastWifiRetryMs >= WIFI_RETRY_INTERVAL_MS) {
    lastWifiRetryMs = now;
    connectWiFi();
  }
#endif
  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    tempSensor.requestTemperatures();
    float t = tempSensor.getTempCByIndex(0);
    latestTemperature = (t == DEVICE_DISCONNECTED_C) ? NAN : t;
    latestVibration = readVibration();
    Serial.print("ID="); Serial.print(lastOrganId);
    Serial.print(" T="); Serial.print(latestTemperature);
    Serial.print(" V="); Serial.println(latestVibration);
#if ENABLE_BACKEND
    sendReading(latestTemperature, latestVibration);
#endif
    updateLcd();
  }
  if (now - lastLcdMs >= LCD_REFRESH_INTERVAL_MS) {
    lastLcdMs = now;
    if (transportActive && remainingSynced) updateLcd();
  }
}

