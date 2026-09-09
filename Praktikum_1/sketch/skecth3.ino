const int buttonPin = 4;
const int ledPin = 5;

int buttonState = 0;
int ledState = LOW;

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);

  digitalWrite(ledPin, LOW); // Awalnya LED mati
}

void loop() {
  buttonState = digitalRead(buttonPin);

  if (buttonState == HIGH) {
    ledState = HIGH; // Tombol ditekan → LED menyala
    digitalWrite(ledPin, ledState);
    Serial.println("Tombol ditekan! -> LED ON");
  }
}