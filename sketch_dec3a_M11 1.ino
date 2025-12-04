#include <WiFi.h>
#include <Firebase_ESP_Client.h>

const char* ssid = "namawifianda";
const char* password = "passwordwifianda":

#define API_KEY "AIzaSyAi7a-XgFYxeADzgPVxqdE9QqGVM1nPZj0"
#define DATABASE_URL "https://tugas-m11-c8b0d-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "william@gmail.com"
#define USER_PASSWORD "123456"

#define dht 23
#define ldr 19
#define soil 18

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== SMART PLANT GREENHOUSE ===");
  Serial.println("Inisialisasi sistem...\n");

  pinMode(LDR_PIN, INPUT);
  pinMode(SOIL_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(FLAME_PIN, INPUT);
  pinMode(OBJECT_PIN, INPUT);

  connectWiFi();
  configTime(gmtOffset_sec, datlightOffset_sec, ntpServer);
  Serial.println("Sinkronisasi waktu dengan NTP...");
  delay(2000);

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD; 
  config.token_status_callback = tokenStatusCallback;
  Serial.printIn("Menghubungkan ke Firbase...");
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  unsigned long fbStart = millis();
  while (!Firebase.ready() &&millis() - fbStart < 10000) {
    Serial.print(".");
    delay(500);
  }
  if (Firebase.ready()) {
    Serial.println("\n✓ Firebase terhubung!");
    Serial.println("\n✓ Sistem siap monitoring!\n");
  } else {
    Serial.println("\nX Firebase gagal terhubung, sistem tetap berjalan...\n");
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) { 
    Serial.println("WiFi terputus! Mencoba reconnect..."); 
    connectWiFi();
}
 
  unsigned long now = millis();
  if (now lastSensorUpdate > sensorInterval) {
    lastSensorUpdate = now; 
    bacaDanKirimData();
  }
}
 
void connectWiFi() {
  WiFi.begin (WIFI_SSID, WIFI_PASSWORD);
  Serial.print ("Menghubungkan ke WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay (500);
    if (millis() start > 20000) { 
      Serial.println("\nX Gagal terhubung WiFi - restart...");
      ESP.restart();
 }
}
Serial.println();
Serial.println("✓ WiFi Terhubung!");
Serial.print("IP Address: ");
Serial.println(WiFi.localIP()); 
}

unsigned long getTimestamp() {
  time_t now;
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) {
    Serial.println("! Gagal mendapat NTP, gunakan millis()");
    return millis();
  }
  time(&now);
  return (unsigned long) now * 1000;
}

void bacaDanKirimData() {
  Serial.println("\n PEMBACAAN SENSOR GREENHOUSE");
  int rawLdr = analogRead(LDR_PIN);
  lightLevel = map(rawLdr, 4095, 0, 0, 100);
  lughtLevel = constrain(lightLevel, 0, 100);

  Serial.printf(" Cahaya: %d %% (ADC=%d)\n", lightLevel, rawLdr);

  int rawSoil = analogRead(SOIL_PIN);
  soilPercent = map(rawSoil, 4095, 0, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  Serial.printf("Kelembaban Tanah: %d %% (ADC=%d)\n", soilPercent, rawSoil);
  if (soilPercent <40) {
    Serial.printIn("! STATUS: KERING - PERLU PENYIRAMAN!");
  } else {
    Serial.printIn("✓ STATUS: KELEMBABAN CUKUP.");
  }

  motionDetected = digitalRead(PIR_PIN) == HIGH;
  flameDetected = digitalRead(FLAME_PIN) == HIGH;
  objectDetected = digitalRead(OBJECT_PIN) == HIGH;

  Serial.printf("Gerakan (PIR): %s\n", motionDetected ? "TERDETEKSI" : "Tidak ada");
  Serial.printf("Api: %s\n", flameDetected ? "TERDETEKSI" : "Aman");
  Serial.printf("Objek: %s\n", objectDetected ? "TERDETEKSI" : "Tidak ada");

  if (Firebase.ready()) {
    Serial.println("\n Mengirim data ke Firebase...");

    String basePath = "/greenhouse/sensors";
    bool allSuccess = true;

    if (Firebase.RTDB.setInt(&fbdo, basePath + "/soilMoisture", soilPercent)) {
      Serial.println("✓ soilMoisture terkirim");
    } else {
      Serial.printf("X soilMoisture gagal: %s\n", fbdo.errorReason().c_str());
      allSuccess = false;
    }

    if (Firebase.RTDB.setBool(&fbdo, basePath + "/motion", motionDetected)) {
      Serial.println("✓ Motion terkirim");
    } else {
      Serial.printf("X Motion gagal: %s\n", fbdo.errorReason().c_str)();
      allSuccess = false;
    }

    if (Firebase.RTDB.setBool(&fbdo, basePath + "/flame", flameDetected)) {
      Serial.println("✓ Flame terkirim");
    } else {
      Serial.printf("X Flame gagal: %s\n", fbdo.errorReason().c_str());
      allSuccess = false;
    }

    if (Firebase.RTDB.setBool(&fbdo, basePath + "/object", objectDetected)) {
      Serial.println("✓ Object terkirim");
    } else {
      Serial.printf("X Object gagal: %s\n", fbdo.errorReason().c_str());
      allSuccess = false;
    }

    unsigned long timestamp = getTimestamp();
    if (Firebase.RTDB.setDouble(&fbdo, basePath + "/timestamp", timestamp)) {
      Serial.printf("✓ Timestamp terkirim (%lu)\n", timestamp);
    } else {
      Serial.printf("X Timestamp gagal: %s\n", fbdo.errorReason().c_str());
      allSuccess = false;
    }

    if(allSuccess) {
      Serial.println("\n Semua data berhasil dikirim!");
    } else {
      Serial.println("\n Beberapa data gagal dikirim");
    }
  } else {
    Serial.println("\n Firebase belum siap, skip pengiriman");
  }

  Serial.println("---------------\n");

  delay(100);
}
