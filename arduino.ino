#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>

// ===== WiFi =====
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASS ""

// ===== Firebase =====
// Isi dari Project Settings (Web API Key) & Realtime Database URL
#define API_KEY       "ISI_API_KEY_KAMU"
#define DATABASE_URL  "https://ISI-PROJECT-KAMU-default-rtdb.firebaseio.com/"

// Email/Password (Authentication -> Sign-in method -> Email/Password -> buat user testing)
#define USER_EMAIL    "emailkamu@gmail.com"
#define USER_PASS     "passwordkamu"

// ===== Sensor =====
#define DHTPIN  15        // sesuaikan pin
#define DHTTYPE DHT11     // ganti ke DHT22 kalau pakai DHT22
DHT dht(DHTPIN, DHTTYPE);

#define LDR_PIN 34        // pin analog ESP32 (GPIO34/35/36/39 aman untuk ADC)

// ===== Firebase objects =====
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ===== Path RTDB =====
String basePath = "/iot";              // contoh folder
String tempPath = basePath + "/temp";
String humPath  = basePath + "/hum";
String ldrPath  = basePath + "/ldr";
String tsPath   = basePath + "/ts";    // timestamp (millis)

unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL_MS = 2000;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void initFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASS;

  Firebase.reconnectWiFi(true);

  Serial.println("Connecting Firebase...");
  Firebase.begin(&config, &auth);

  // Tunggu token siap
  while (auth.token.uid == "") {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nFirebase ready!");
}

void setup() {
  Serial.begin(115200);
  delay(200);

  dht.begin();
  connectWiFi();
  initFirebase();
}

void loop() {
  // jaga WiFi tetap tersambung
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi putus, reconnect...");
    connectWiFi();
  }

  if (millis() - lastSend >= SEND_INTERVAL_MS) {
    lastSend = millis();

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int ldrRaw = analogRead(LDR_PIN);

    if (isnan(h) || isnan(t)) {
      Serial.println("Gagal baca DHT!");
      return;
    }

    Serial.printf("Temp: %.2f C | Hum: %.2f %% | LDR: %d\n", t, h, ldrRaw);

    // Kirim ke RTDB (SET)
    bool ok1 = Firebase.RTDB.setFloat(&fbdo, tempPath.c_str(), t);
    bool ok2 = Firebase.RTDB.setFloat(&fbdo, humPath.c_str(), h);
    bool ok3 = Firebase.RTDB.setInt(&fbdo, ldrPath.c_str(), ldrRaw);
    bool ok4 = Firebase.RTDB.setInt(&fbdo, tsPath.c_str(), (int)millis());

    if (ok1 && ok2 && ok3 && ok4) {
      Serial.println("Update RTDB: OK");
    } else {
      Serial.print("Update RTDB: GAGAL -> ");
      Serial.println(fbdo.errorReason());
    }
  }
}
