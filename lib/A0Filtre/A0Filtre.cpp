#include "A0Filtre.h"
#include "Config.h"

uint16_t a0FiltreliOku() {
  const uint8_t N = A0_FILTER_SAMPLES;
  static uint16_t buf[A0_FILTER_SAMPLES]; // sabit boyutlu; 50 default

  pinMode(OPKON_PIN, INPUT);

  for (uint8_t i = 0; i < N; i++) {
    buf[i] = analogRead(OPKON_PIN);
    if (A0_FILTER_SPACING_US > 0) {
      delayMicroseconds(A0_FILTER_SPACING_US);
    }
  }

  // Basit mod hesaplama (N küçükken yeterli)
  uint16_t modVal = buf[0];
  uint8_t  modCnt = 1;

  for (uint8_t i = 0; i < N; i++) {
    uint8_t cnt = 1;
    for (uint8_t j = i + 1; j < N; j++) {
      if (buf[j] == buf[i]) cnt++;
    }
    if (cnt > modCnt) {
      modCnt = cnt;
      modVal = buf[i];
    }
  }
  return modVal;
}
