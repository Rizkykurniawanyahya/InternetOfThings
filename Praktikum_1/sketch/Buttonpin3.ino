/*
  ESP8266 Blink by Simon Peter
  Blink the blue LED on the ESP-01 module
  This example code is in the public domain

  The blue LED on the ESP-01 module is connected to GPIO1
  (which is also the TXD pin; so we cannot use Serial.print() at the same time)

  Note that this sketch uses LED_BUILTIN to find the pin with the internal LED
*/
const int buttonPin = 4;
const int ledPin = 5;

int buttonState = 0;
int lastButtonState = LOW;
int ledState = LOW;

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  buttonState = digitalRead(buttonPin);

  // Mendeteksi tombol baru ditekan
  if (buttonState == HIGH && lastButtonState == LOW) {

    // Membalik status LED
    ledState = !ledState;

    digitalWrite(ledPin, ledState);

    if (ledState == HIGH) {
      Serial.println("Tombol ditekan! -> LED ON");
    } else {
      Serial.println("Tombol ditekan! -> LED OFF");
    }

    // Debouncing
    delay(200);
  }

  lastButtonState = buttonState;
}