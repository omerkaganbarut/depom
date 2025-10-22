// KayitModulu.h - FINAL VERSION
#ifndef KAYITMODULU_H
#define KAYITMODULU_H

#include <Arduino.h>
#include "stepmotorenkoderiokuma.h"

// ═══════════════════════════════════════════════════════════════
// KAYIT VERİ YAPISI
// ═══════════════════════════════════════════════════════════════
struct KM_Sample {
  long enc;
  uint16_t a0;
};

// ═══════════════════════════════════════════════════════════════
// FONKSİYON TANIMLARI
// ═══════════════════════════════════════════════════════════════

void kayitEncoderSetup(StepMotorEncoder* bigEncoder);
void kayitBaslat(int yon);
void kayitRun();
bool kayitAktifMi();
bool kayitTamamlandiMi();
uint16_t kayitOrnekSayisi();

// Read-only versiyon
const KM_Sample* kayitVerileri();

// ✅ YENİ: Düzenleme için non-const versiyon
KM_Sample* kayitVerileriDuzenle();

void kayitListele();
void kayitDurdur();

#endif // KAYITMODULU_H