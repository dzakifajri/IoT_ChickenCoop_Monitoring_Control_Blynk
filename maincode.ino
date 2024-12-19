#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL61L-VGdFd"
#define BLYNK_TEMPLATE_NAME "Monitoring Ternak Ayam"
#define BLYNK_AUTH_TOKEN "h8oKWgV-cfP5woWoWB99cdHWU9R9IMVT"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <SimpleTimer.h> // Timer untuk loop

// Pin konfigurasi
#define DHTPIN 4           // Pin DATA DHT11
#define DHTTYPE DHT11      // Tipe sensor DHT
#define relayKipas 25      // GPIO untuk relay kipas
#define relayLampu 26     // GPIO untuk relay lampu

// Inisialisasi DHT, RTC, dan LCD
DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2); // Alamat I2C LCD biasanya 0x27
SimpleTimer timer;

// Blynk WiFi credentials
char ssid[] = "jek";          // WiFi SSID
char pass[] = "12345677";     // WiFi Password
char auth[] = "h8oKWgV-cfP5woWoWB99cdHWU9R9IMVT"; // Blynk Auth Token

// Blynk Virtual Pins
#define VIRTUAL_PIN_SUHU 1
#define VIRTUAL_PIN_KELEMBAPAN 2
#define VIRTUAL_PIN_LAMPU 0
#define VIRTUAL_PIN_KIPAS 3
#define VIRTUAL_PIN_RTC 4
#define VIRTUAL_PIN_PERIODE 5

// Variabel untuk periode kontrol
unsigned long periodStartTime = 0;
int period = 1;
float targetSuhu = 25;         // Target suhu awal
float targetKelembapan = 75;   // Target kelembapan awal

void setup() {  
  // Inisialisasi Serial Monitor
  Serial.begin(115200);
  Serial.println("Memulai program...");

  // Inisialisasi sensor DHT
  dht.begin();
  Serial.println("Sensor DHT siap!");

  // Inisialisasi RTC DS3231
  if (!rtc.begin()) {
    Serial.println("RTC tidak terdeteksi!");
    while (1) delay(1000); // Berhenti jika RTC tidak terdeteksi
  }
  if (rtc.lostPower()) {
    Serial.println("RTC kehilangan daya, menyetel ulang waktu.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Sesuaikan waktu ke waktu kompilasi
  }

  // Inisialisasi LCD
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Kandang");
  lcd.setCursor(0, 1);
  lcd.print("Ayam Pintar Mulai");
  delay(2000); // Tampilkan pesan selama 2 detik

  // Inisialisasi pin relay
  pinMode(relayKipas, OUTPUT);
  pinMode(relayLampu, OUTPUT);
  digitalWrite(relayKipas, LOW);
  digitalWrite(relayLampu, LOW);

  // Koneksi WiFi
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  // Koneksi Blynk
  Serial.println("Connecting to Blynk...");
  Blynk.config(auth);
  while (!Blynk.connect()) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nBlynk connected!");

  // Set timer untuk pengiriman data dan update periode
  timer.setInterval(5000L, sendSensorData); // Kirim data setiap 5 detik
  timer.setInterval(300000L, updatePeriod); // Update target setiap 5 menit
  timer.setInterval(5000L, sendRTCData);    // Kirim data waktu setiap 5 detik
}

void loop() {
  Blynk.run();
  timer.run();
}

void updateLCD() {
  lcd.clear(); // Bersihkan layar LCD

  // Baris pertama: Tampilkan periode
  lcd.setCursor(0, 0);
  lcd.print("Periode: ");
  lcd.print(period);

  // Baris kedua: Tampilkan status sistem
  lcd.setCursor(0, 1);
  lcd.print("Sistem: ");
  if (digitalRead(relayKipas) == HIGH || digitalRead(relayLampu) == HIGH) {
    lcd.print("ON");
  } else {
    lcd.print("OFF");
  }
}

void updatePeriod() {
  unsigned long elapsedTime = millis() - periodStartTime;

  if (elapsedTime > 300000L) { // Jika lebih dari 5 menit
    period++; // Naikkan periode
    periodStartTime = millis(); // Reset waktu awal periode

    // Jika periode lebih dari 5, kembali ke periode 1
    if (period > 5) {
      period = 1;
    }

    // Atur target berdasarkan periode
    switch (period) {
      case 1:
        targetSuhu = 26;
        targetKelembapan = 50;
        break;
      case 2:
        targetSuhu = 29;
        targetKelembapan = 50;
        break;
      case 3:
      case 4:
      case 5:
        targetSuhu = 27;
        targetKelembapan = 55;
        break;
    }

    // Debugging untuk monitoring
    Serial.print("Periode ke-");
    Serial.print(period);
    Serial.print(" | Target Suhu: ");
    Serial.print(targetSuhu);
    Serial.print(" °C | Target Kelembapan: ");
    Serial.println(targetKelembapan);

    // Perbarui tampilan LCD
    updateLCD();
  }
}

void sendRTCData() {
  DateTime now = rtc.now();
  String waktu = String(now.hour()) + ":" +
                 (now.minute() < 10 ? "0" : "") + String(now.minute()) + ":" +
                 (now.second() < 10 ? "0" : "") + String(now.second());

  // Kirim waktu ke Virtual Pin 4
  Blynk.virtualWrite(VIRTUAL_PIN_RTC, waktu);
}

void sendSensorData() {
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();

  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println("Gagal membaca data DHT11!");
    return;
  }

  // Kontrol relay berdasarkan suhu dan kelembapan
  bool kipasStatus = false;
  bool lampuStatus = false;

  if (suhu > targetSuhu ){
    digitalWrite(relayKipas, HIGH);
    digitalWrite(relayLampu, LOW);
    kipasStatus = true;
    lampuStatus = false;
  } else {
    digitalWrite(relayKipas, LOW);
    digitalWrite(relayLampu, HIGH);
    kipasStatus = false;
    lampuStatus = true;
  }

  // Debugging pada Serial Monitor
  Serial.println("-------------------------------");
  Serial.print("Waktu: ");
  DateTime now = rtc.now();
  Serial.println(String(now.hour()) + ":" + 
                 (now.minute() < 10 ? "0" : "") + String(now.minute()) + ":" +
                 (now.second() < 10 ? "0" : "") + String(now.second()));

  Serial.print("Suhu: ");
  Serial.print(suhu);
  Serial.print(" °C  |  Kelembapan: ");
  Serial.print(kelembapan);
  Serial.println(" %");

  Serial.print("Kipas: ");
  Serial.print(kipasStatus ? "ON" : "OFF");
  Serial.print("  |  Lampu: ");
  Serial.println(lampuStatus ? "ON" : "OFF");

  Serial.print("Periode: ");
  Serial.println(period);
  Serial.println("-------------------------------");

  // Kirim data ke Blynk
  Blynk.virtualWrite(VIRTUAL_PIN_SUHU, suhu);
  Blynk.virtualWrite(VIRTUAL_PIN_KELEMBAPAN, kelembapan);
  Blynk.virtualWrite(VIRTUAL_PIN_LAMPU, digitalRead(relayLampu));
  Blynk.virtualWrite(VIRTUAL_PIN_KIPAS, digitalRead(relayKipas));
  Blynk.virtualWrite(VIRTUAL_PIN_PERIODE, period);

  // Perbarui tampilan LCD
  updateLCD();
}
