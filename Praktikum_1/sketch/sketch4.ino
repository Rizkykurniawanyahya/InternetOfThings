const int buttonPin = 4;
const int ledPin = 5;

int buttonState = LOW;
int lastButtonState = LOW;
int ledState = LOW;

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);

  digitalWrite(ledPin, LOW); // Awalnya LED mati
}

void loop() {
  buttonState = digitalRead(buttonPin);

  // Tombol baru saja ditekan
  if (buttonState == HIGH && lastButtonState == LOW) {
    ledState = !ledState; // Membalik kondisi LED

    digitalWrite(ledPin, ledState);

    if (ledState == HIGH) {
      Serial.println("LED ON");
    } else {
      Serial.println("LED OFF");
    }

    delay(50); // debounce sederhana
  }

  lastButtonState = buttonState;
}