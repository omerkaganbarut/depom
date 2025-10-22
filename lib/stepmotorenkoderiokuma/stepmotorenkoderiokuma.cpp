// stepmotorenkoderiokuma.cpp
#include "stepmotorenkoderiokuma.h"

StepMotorEncoder* StepMotorEncoder::instances[3] = {nullptr, nullptr, nullptr};  // ✅ 3 slot

StepMotorEncoder::StepMotorEncoder(uint8_t pinA, uint8_t pinB) : _pinA(pinA), _pinB(pinB) {}

void StepMotorEncoder::begin() {
  pinMode(_pinA, INPUT_PULLUP);
  pinMode(_pinB, INPUT_PULLUP);
  _lastB = digitalRead(_pinB);

  for (int i = 0; i < 3; i++) {  // ✅ 3 encoder
    if (!instances[i]) {
      instances[i] = this;
      switch (i) {
        case 0: attachInterrupt(digitalPinToInterrupt(_pinA), isrRouter0, CHANGE); break;
        case 1: attachInterrupt(digitalPinToInterrupt(_pinA), isrRouter1, CHANGE); break;
        case 2: attachInterrupt(digitalPinToInterrupt(_pinA), isrRouter2, CHANGE); break;
      }
      break;
    }
  }
}

long StepMotorEncoder::getPosition() const {
  noInterrupts();
  long val = _position;
  interrupts();
  return val;
}

void StepMotorEncoder::handleInterrupt() {
  bool A = digitalRead(_pinA);
  bool B = digitalRead(_pinB);
  _position += (A == _lastB) ? +1 : -1;
  _lastB = B;
}

void StepMotorEncoder::isrRouter0() {
  if (instances[0]) instances[0]->handleInterrupt();
}

void StepMotorEncoder::isrRouter1() {
  if (instances[1]) instances[1]->handleInterrupt();
}

void StepMotorEncoder::isrRouter2() {
  if (instances[2]) instances[2]->handleInterrupt();
}

/*


               ??? NASIL KULLANILIR MAİN C DE???

#include <Arduino.h>
#include "stepmotorenkoderiokuma.h"

// --- PIN SEÇİMİ (Arduino Mega) ---
// A fazı → interrupt destekli pin: D2, D3, D18–21
// B fazı → herhangi bir dijital pin

// Encoder nesneleri
StepMotorEncoder stepEnc1(21, 4);  // A: D21, B: D4
StepMotorEncoder stepEnc2(20, 5);  // A: D20, B: D5
StepMotorEncoder stepEnc3(19, 6);  // A: D19, B: D6

void setup() {
  Serial.begin(115200);

  stepEnc1.begin(); // pinMode + interrupt
  stepEnc2.begin();
  stepEnc3.begin();
}

void loop() {
  static unsigned long lastT = 0;

  if (millis() - lastT >= 250) {
    // Her encoder için pozisyonu oku
    Serial.print("ENC1: "); Serial.print(stepEnc1.getPosition());
    Serial.print(" | ENC2: "); Serial.print(stepEnc2.getPosition());
    Serial.print(" | ENC3: "); Serial.println(stepEnc3.getPosition());
    lastT = millis();
  }
}
*/