#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// Kredensial Jaringan Wi-Fi
const char* ssid = "SAMBIL LALU";
const char* password = "martabakmanis";

// Konfigurasi Broker MQTT Publik
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* clientId = "ESP8266_LabIoT_25782110_Full";

// Pembagian Topik Mandiri
const char* topicPublish   = "kampus/labiot/25782110/telemetri";
const char* topicSubscribe = "kampus/labiot/25782110/kendali";
const char* topicAlert     = "kampus/labiot/25782110/alert";
const char* topicStatus    = "kampus/labiot/25782110/status";

// Pesan status. LWT = pesan wasiat yang dikirim broker jika perangkat putus mendadak
const char* pesanLwt    = "{\"status\":\"OFFLINE\",\"keterangan\":\"Perangkat Terputus Tak Terduga\"}";
const char* pesanOnline = "{\"status\":\"ONLINE\",\"keterangan\":\"Perangkat Beroperasi Normal\"}";

// Ambang batas suhu bahaya dan flag penanda (state latch)
const float batasSuhu = 31.0;
bool alertAktif = false;     // true selama suhu di atas ambang batas
bool alertTerkirim = false;  // true jika alert sudah terkirim (agar tidak membanjiri broker)

// Alokasi Pin Perangkat Keras
const byte dhtPin = 2; // Pin D4 (GPIO 2)
const byte ledPin = 5; // Pin D1 (GPIO 5)
#define DHTTYPE DHT22

DHT dht(dhtPin, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

// Penjadwalan non-blocking (tanpa delay)
unsigned long lastTelemetryTime = 0;
const unsigned long telemetryInterval = 4000; // Siklus kirim 4 detik
unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 4000; // Coba konek MQTT tiap 4 detik

bool wifiInfoSudahTercetak = false;

// ===================== CALLBACK KENDALI =====================
void callback(char* topic, byte* payload, unsigned int length) {
  char incomingBuffer[length + 1];
  memcpy(incomingBuffer, payload, length);
  incomingBuffer[length] = '\0';

  Serial.println("----------------------------------------");
  Serial.printf("[KONTROL DITERIMA] Topik: %s | Payload: %s\n", topic, incomingBuffer);

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, incomingBuffer);
  if (err) {
    Serial.println("Gagal membedah payload JSON!");
    return;
  }

  if (doc.containsKey("led")) {
    int val = doc["led"];
    // Saat bahaya, LED dikunci menyala. Perintah mematikan ditolak.
    if (alertAktif && val != 1) {
      Serial.println("--> [DITOLAK] Suhu BAHAYA, LED dikunci MENYALA.");
    } else {
      digitalWrite(ledPin, val == 1 ? HIGH : LOW);
      Serial.printf("--> [INDIKATOR] Status LED diubah: %s\n", val == 1 ? "MENYALA" : "PADAM");
    }
  }
  Serial.println("----------------------------------------");
}

// ===================== KONEKSI MQTT (NON-BLOCKING) =====================
// Satu kali percobaan konek. Dipanggil berkala dari loop() memakai millis().
bool connectMqtt() {
  Serial.print("Menghubungkan ke MQTT Broker...");

  // connect(id, user, pass, willTopic, willQos, willRetain, willMessage)
  if (client.connect(clientId, NULL, NULL, topicStatus, 1, true, pesanLwt)) {
    Serial.println(" Sukses Terhubung!");

    client.subscribe(topicSubscribe);
    Serial.printf("Subskripsi Topik: %s\n", topicSubscribe);

    // Umumkan status ONLINE (retained) di topik yang sama dengan LWT
    client.publish(topicStatus, pesanOnline, true);
    Serial.printf("[STATUS] ONLINE dikirim ke %s\n", topicStatus);
    return true;
  }

  Serial.printf(" Gagal (rc=%d)\n", client.state());
  return false;
}

// ===================== KIRIM ALERT =====================
void kirimAlert(float suhu) {
  StaticJsonDocument<128> docAlert;
  docAlert["status"]     = "BAHAYA";
  docAlert["parameter"]  = "OVERHEAT";
  docAlert["nilai_suhu"] = serialized(String(suhu, 1));

  char buffer[128];
  serializeJson(docAlert, buffer);

  if (client.publish(topicAlert, buffer)) {
    alertTerkirim = true; // kunci: jangan kirim lagi sampai suhu kembali normal
    Serial.printf("[ALERT TERKIRIM] %s\n", buffer);
  } else {
    Serial.println("[ERROR] Gagal mengirim alert, akan dicoba lagi.");
  }
}

// ===================== BACA SENSOR + RULE ENGINE + TELEMETRI =====================
void bacaDanProses() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("[PERINGATAN] Gagal membaca data DHT!");
    return;
  }

  bool panas = (t > batasSuhu);

  // ---- Edge safety engine (berjalan lokal) ----
  if (panas && !alertAktif) {
    alertAktif = true;
    alertTerkirim = false;
    digitalWrite(ledPin, HIGH); // LED bahaya menyala otomatis
    Serial.println("[BAHAYA] Suhu melewati ambang batas, LED dinyalakan otomatis!");
  } else if (!panas && alertAktif) {
    alertAktif = false;
    alertTerkirim = false;
    digitalWrite(ledPin, LOW);
    Serial.println("[INFO] Suhu kembali normal, LED padam, kendali manual aktif.");
  }
  if (client.connected()) {
  StaticJsonDocument<128> docPulih;
  docPulih["status"] = "NORMAL";
  docPulih["parameter"] = "OVERHEAT";
  docPulih["nilai_suhu"] = serialized(String(t, 1));
  char bufPulih[128];
  serializeJson(docPulih, bufPulih);
  client.publish(topicAlert, bufPulih);
  }

  // Kirim alert sekali saja saat bahaya terdeteksi pertama kali
  if (alertAktif && !alertTerkirim && client.connected()) {
    kirimAlert(t);
  }

  // ---- Telemetri rutin ----
  if (client.connected()) {
    StaticJsonDocument<384> docOut;
    docOut["node_id"]      = clientId;
    docOut["suhu"]         = serialized(String(t, 2));
    docOut["kelembapan"]   = serialized(String(h, 2));
    docOut["status_led"]   = digitalRead(ledPin);
    docOut["kondisi_suhu"] = panas ? "PANAS" : "NORMAL";
    docOut["uptime_detik"] = millis() / 1000;        // lama sistem menyala (detik)
    docOut["ip_lokal"]     = WiFi.localIP().toString();

    char buffer[384];
    serializeJson(docOut, buffer);

    if (client.publish(topicPublish, buffer)) {
      Serial.printf("[TELEMETRI TERKIRIM] %s\n", buffer);
    } else {
      Serial.println("[ERROR] Gagal memublikasikan telemetri!");
    }
  }
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); // konek berjalan di latar, dicek dari loop()
  Serial.println("Menghubungkan Wi-Fi...");

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  client.setBufferSize(512);  // jaga-jaga agar paket JSON tidak terpotong
}

// ===================== LOOP =====================
void loop() {
  unsigned long now = millis();

  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiInfoSudahTercetak) {
      Serial.println("Wi-Fi Terkoneksi!");
      Serial.print("Alamat IP Lokal: ");
      Serial.println(WiFi.localIP());
      wifiInfoSudahTercetak = true;
    }

    if (!client.connected()) {
      if (now - lastReconnectAttempt >= reconnectInterval) {
        lastReconnectAttempt = now;
        connectMqtt();
      }
    } else {
      client.loop(); // Memelihara sesi TCP dan memproses callback masuk
    }
  } else {
    wifiInfoSudahTercetak = false;
  }

  // Jadwal baca sensor + rule engine + telemetri
  if (now - lastTelemetryTime >= telemetryInterval) {
    lastTelemetryTime = now;
    bacaDanProses();
  }
}
