#define BLYNK_TEMPLATE_ID "TMPL6KJkvLqNY"
#define BLYNK_TEMPLATE_NAME "GoKillS Project"
#define BLYNK_AUTH_TOKEN "CKZJXYa424HI9Bk9Q-NyZs6l8KXe7cws"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>  // Library untuk HTTP client
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int mq135Pin = 35; // Pin sensor MQ135
const int ledPin = 27;   // Pin LED merah
const int buzzerPin = 26; // Pin Buzzer
const float gasThreshold = 50.0; // Menyesuaikan dengan PEL dari OSHA

char ssid[] = "@wiFi.iD_Ext"; // Ganti dengan SSID WiFi Anda
char pass[] = "waduhlaliaku"; // Ganti dengan kata sandi WiFi Anda

String deviceName = "Kamar Mandi Ibnu"; // Nama perangkat untuk notifikasi

// HTTP API untuk Telegram
const char* telegramBotToken = "6558496248:AAG08hBSZQVpi0ys7SnzvtsHvX2YwP2UmOU"; // Ganti dengan token bot Telegram Anda
const char* telegramChatId = "-4240811742"; // Ganti dengan chat ID Anda

void setup() {
  pinMode(mq135Pin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  Serial.begin(115200);

  // Menghubungkan ke WiFi dan Blynk
  WiFi.begin(ssid, pass);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(2); // Memperbesar ukuran teks untuk OLED
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("GoKillS"));
  display.println("Ajibb Gann");
  display.display();
  delay(4000);
}

void sendMessageTelegram(float ppm) {
  HTTPClient http;
  String url = "https://api.telegram.org/bot" + String(telegramBotToken) + "/sendMessage";

  // Buat objek JSON untuk pesan
  StaticJsonDocument<200> doc;
  doc["chat_id"] = telegramChatId;
  doc["text"] = "Wc di lokasi " + String(deviceName) + " memerlukan perhatian, konsentrasi amonia terdeteksi " + String(ppm) + " ppm.\n\nKondisi: Bau.";

  // Serialize JSON ke String
  String jsonPayload;
  serializeJson(doc, jsonPayload);

  // Mengatur header HTTP
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // Kirim HTTP POST request dengan payload JSON
  int httpResponseCode = http.POST(jsonPayload);

  // Tanggapan HTTP
  if (httpResponseCode > 0) {
    Serial.print("Telegram message sent successfully. Response code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Error sending Telegram message. HTTP error code: ");
    Serial.println(httpResponseCode);
  }

  // Menutup koneksi
  http.end();
}

void loop() {
  Blynk.run(); // Menjalankan Blynk

  int analogValue = analogRead(mq135Pin);
  float voltage = analogValue * (3.3 / 4095.0);
  float gasConcentration = (voltage - 0.1) * 100; // Kalibrasi sederhana
  float ammoniaConcentration = (voltage - 0.1) * 50; // Kalibrasi sederhana untuk amonia

  // Hitung persentase konsentrasi gas
  int ammoniaPercentage = (ammoniaConcentration / gasThreshold) * 100.0;
  if (ammoniaPercentage > 100.0) ammoniaPercentage = 100.0; // Pastikan tidak lebih dari 100%

  // Menentukan kondisi berdasarkan konsentrasi gas
  String kondisi;
  if (ammoniaConcentration <= 32) {
    kondisi = "Aman";
  } else if (ammoniaConcentration <= 50) {
    kondisi = "Normal";
  } else {
    kondisi = "Bau";
  }

  // Debugging output
  Serial.print("Analog Value: ");
  Serial.print(analogValue);
  Serial.print(" - Voltage: ");
  Serial.print(voltage);
  Serial.print(" V - Gas Concentration: ");
  Serial.print(gasConcentration);
  Serial.print(" ppm - Ammonia Concentration: ");
  Serial.print(ammoniaConcentration);
  Serial.print(" ppm - Ammonia Percentage: ");
  Serial.print(ammoniaPercentage);
  Serial.print(" % - Kondisi: ");
  Serial.println(kondisi);

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Kadar NH3:");
  display.print(ammoniaConcentration);
  display.println(" ppm");
  display.print("Level:");
  display.print(ammoniaPercentage);
  display.println("%");
  display.print(kondisi);
  display.display();

  // Kirim data ke Blynk
  Blynk.virtualWrite(V0, gasConcentration); // V0 untuk kadar gas dalam ppm
  Blynk.virtualWrite(V1, kondisi); // V1 untuk kondisi gas
  Blynk.virtualWrite(V3, ammoniaConcentration); // V3 untuk kadar amonia dalam ppm

  if (kondisi == "Bau") {
    digitalWrite(ledPin, HIGH);
    digitalWrite(buzzerPin, HIGH);
    Blynk.virtualWrite(V2, 1); // V2 untuk menyalakan LED di Blynk
    sendMessageTelegram(ammoniaConcentration); // Mengirim notifikasi Telegram jika kondisi "Bau"
  } else {
    digitalWrite(ledPin, LOW);
    digitalWrite(buzzerPin, LOW);
    Blynk.virtualWrite(V2, 0); // V2 untuk mematikan LED di Blynk
  }

  delay(5000);
}
