#include <DHT.h>

#define DHTPIN D7
#define DHTTYPE DHT22

const int ldrPin = A0;
const int relayPin = D6;
const int ledPin = D1;

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);

  dht.begin();

  pinMode(relayPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  // Kondisi awal: aktuator mati
  digitalWrite(relayPin, HIGH);
  digitalWrite(ledPin, LOW);

  Serial.println("Sistem Monitoring Dimulai");
}

void loop() {
  float suhu = dht.readTemperature();
  int ldrValue = analogRead(ldrPin);

  // Cek apakah pembacaan DHT valid
  if (isnan(suhu)) {
    Serial.println("Gagal membaca DHT22!");
    delay(2000);
    return;
  }

  Serial.print("Suhu: ");
  Serial.print(suhu);
  Serial.print(" C | LDR: ");
  Serial.println(ldrValue);

  // Suhu > 34 ATAU LDR < 300
  if (suhu > 40|| ldrValue < 500) {

    // Untuk relay Active-Low
    digitalWrite(relayPin, LOW);
    digitalWrite(ledPin, HIGH);

    Serial.println("Peringatan: Aktuator Aktif!");

  } else {

    // Relay mati
    digitalWrite(relayPin, HIGH);
    digitalWrite(ledPin, LOW);

    Serial.println("Kondisi Aman");
  }

  Serial.println("-------------------------");

  delay(2000);
}