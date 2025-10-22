// stepmotorenkoderiokuma.h
#ifndef STEPMOTORENKODERIOKUMA_H
#define STEPMOTORENKODERIOKUMA_H

#include <Arduino.h>

class StepMotorEncoder {
public:
  StepMotorEncoder(uint8_t pinA, uint8_t pinB);
  void begin();
  long getPosition() const;
  void reset() {
    noInterrupts();
    _position = 0;
    interrupts();
  }
  void handleInterrupt();

private:
  uint8_t _pinA, _pinB;
  volatile long _position = 0;
  volatile bool _lastB = 0;

  static void isrRouter0();
  static void isrRouter1();
  static void isrRouter2();
  
  static StepMotorEncoder* instances[3];  // ✅ 3 encoder
};

#endif