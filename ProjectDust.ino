#define BLYNK_TEMPLATE_ID "TMPL6eW-8Ymo9"      // Template ID สำหรับ Blynk
#define BLYNK_TEMPLATE_NAME "Quickstart Template" // ชื่อ Template สำหรับ Blynk
#define BLYNK_AUTH_TOKEN "1sqaXIPKwqq6tEJz4Pv2-klG9bjvabMp" // Blynk Authentication Token

#include <Wire.h>                               // ไลบรารี Wire สำหรับ I2C
#include <LiquidCrystal_I2C.h>                  // ไลบรารี LCD สำหรับจอ I2C
#include <SoftwareSerial.h>                     // ไลบรารี SoftwareSerial สำหรับเซ็นเซอร์ PMS5003
#include "PMS.h"                               // ไลบรารี PMS สำหรับเซ็นเซอร์ PM2.5
#include <ESP8266WiFi.h>                        // ไลบรารี WiFi
#include <ESP8266HTTPClient.h>                  // ไลบรารี HTTPClient สำหรับ ESP8266
#include <BlynkSimpleEsp8266.h>                 // ไลบรารี Blynk

#define PMS_RX D6                               // ขา RX ของเซ็นเซอร์ PMS5003
#define PMS_TX D7                               // ขา TX ของเซ็นเซอร์ PMS5003
#define BUZZER_PIN D5                           // ขาสัญญาณ Buzzer
#define PM25_ALERT_LEVEL 100  // ระดับ PM2.5 ที่จะส่งการแจ้งเตือน

LiquidCrystal_I2C lcd(0x27, 16, 2);             // ออบเจ็กต์ LCD
SoftwareSerial pmsSerial(PMS_RX, PMS_TX);       // ออบเจ็กต์ SoftwareSerial
PMS pms(pmsSerial);                             // ออบเจ็กต์ PMS
PMS::DATA data;                                 // ตัวแปรเก็บข้อมูลจากเซ็นเซอร์

char ssid[] = "Chanathip26_2.4G";                   // ชื่อ WiFi
char pass[] = "26092545PG";                      // รหัสผ่าน WiFi

String googleSheetsUrl = "https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec"; // URL ของ Google Apps Script

BlynkTimer timer;                               // ตัวจับเวลา Blynk
bool previousAlert = false;                     // ตัวแปรเก็บสถานะการแจ้งเตือน

void setupNTP() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nTime synchronized!");
}

void sendToGoogleSheets(float pm25) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient wifiClient; // สร้างออบเจ็กต์ WiFiClient
    HTTPClient http;

    http.begin(wifiClient, googleSheetsUrl); // ใช้ออบเจ็กต์ WiFiClient กับ URL
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"pm25\":" + String(pm25) + "}"; // JSON Payload
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      Serial.print("Google Sheets Response: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error sending to Google Sheets: ");
      Serial.println(http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

void sendDataToBlynk() {
  pms.wakeUp();
  delay(1500);

  if (pms.readUntil(data)) {
    float pm25 = data.PM_SP_UG_2_5;
    Blynk.virtualWrite(V4, pm25);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PM2.5:");
    lcd.print(pm25);
    lcd.print("ug/m3");

    if (pm25 > PM25_ALERT_LEVEL) {
      if (!previousAlert) {
        previousAlert = true;
        lcd.setCursor(0, 1);
        lcd.print("WARNING!");
        sendToGoogleSheets(pm25); // ส่งค่าฝุ่นไปยัง Google Sheets
      }
    } else {
      if (previousAlert) {
        previousAlert = false;
        lcd.setCursor(0, 1);
        lcd.print("SAFE       ");
        sendToGoogleSheets(pm25); // ส่งค่าฝุ่นไปยัง Google Sheets
      }
    }
  } else {
    Serial.println("Failed to read PMS5003");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error");
  }

  pms.sleep();
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  setupNTP();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  lcd.begin();
  lcd.backlight();
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pmsSerial.begin(9600);
  pms.passiveMode();

  timer.setInterval(5000L, sendDataToBlynk);
}

void loop() {
  Blynk.run();
  timer.run();
}
