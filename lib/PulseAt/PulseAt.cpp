// PulseAt.cpp - STEP MOTOR PARALEL SÜRÜŞ MODÜLÜ
#include "PulseAt.h"
#include "Config.h"

// ═══════════════════════════════════════════════════════════════
// PIN TABLOLARI
// ═══════════════════════════════════════════════════════════════
static const uint8_t stepPins[3] = STEP_PINS;
static const uint8_t dirPins[3]  = DIR_PINS;
static const uint8_t enaPins[3]  = ENA_PINS;

// ═══════════════════════════════════════════════════════════════
// AKTİF MOTOR
// ═══════════════════════════════════════════════════════════════
static uint8_t aktifMotor = MOTOR_B;

// ═══════════════════════════════════════════════════════════════
// MOTOR DURUM YAPISI
// ═══════════════════════════════════════════════════════════════
struct PulseState {
  bool aktif = false;
  bool bittiEdge = false;
  
  unsigned long lastUs = 0;
  unsigned long periodUs = 0;
  
  unsigned long sayac = 0;
  unsigned long hedef = 0;
  
  uint8_t yon = 0;
};

static PulseState st[3];

// ═══════════════════════════════════════════════════════════════
// PIN HAZIRLIĞI
// ═══════════════════════════════════════════════════════════════
static inline void ensurePins(uint8_t m) {
  pinMode(stepPins[m], OUTPUT);
  pinMode(dirPins[m],  OUTPUT);
  pinMode(enaPins[m],  OUTPUT);
  digitalWrite(enaPins[m], LOW);  // Motor aktif
}

// ═══════════════════════════════════════════════════════════════
// AKTİF MOTOR SEÇİMİ
// ═══════════════════════════════════════════════════════════════
void useMotor(uint8_t motorIndex) {
  if (motorIndex > 2) return;
  aktifMotor = motorIndex;
}

// ═══════════════════════════════════════════════════════════════
// PULSE GÖNDERİMİ
// ═══════════════════════════════════════════════════════════════
void pulseAt(unsigned long toplamPulse, int yon, unsigned int hertz) {
  uint8_t m = aktifMotor;
  PulseState &S = st[m];  // Referans
  
  // ───────────────────────────────────────────────────────────
  // YENİ İŞ BAŞLATMA
  // ───────────────────────────────────────────────────────────
  if (!S.aktif && toplamPulse > 0 && hertz > 0) {
    ensurePins(m);
    
    digitalWrite(dirPins[m], yon ? HIGH : LOW);
    
    S.periodUs = 1000000UL / hertz;
    S.hedef = toplamPulse;
    S.sayac = 0;
    S.yon = (uint8_t)yon;
    S.lastUs = micros();
    S.aktif = true;
    S.bittiEdge = false;
    
    return;
  }
  
  // ───────────────────────────────────────────────────────────
  // ARKA PLAN
  // ───────────────────────────────────────────────────────────
  if (!S.aktif) return;
  
  unsigned long now = micros();
  
  if (now - S.lastUs >= S.periodUs) {
    // ✅ DRİFT ÖNLEME: lastUs'a periyot ekle
    S.lastUs += S.periodUs;
    
    digitalWrite(stepPins[m], HIGH);
    delayMicroseconds(PULSE_HIGH_US);
    digitalWrite(stepPins[m], LOW);
    
    if (++S.sayac >= S.hedef) {
      S.aktif = false;
      S.bittiEdge = true;
      // ✅ ENA HIGH YAPMA! (Segment geçişlerinde)
    }
  }
}

// ═══════════════════════════════════════════════════════════════
// AKTİF Mİ?
// ═══════════════════════════════════════════════════════════════
bool pulseAtAktifMi(uint8_t motorIndex) {
  if (motorIndex > 2) return false;
  return st[motorIndex].aktif;
}

// ═══════════════════════════════════════════════════════════════
// BİTTİ Mİ?
// ═══════════════════════════════════════════════════════════════
bool pulseAtBittiMi(uint8_t motorIndex) {
  if (motorIndex > 2) return false;
  
  bool bitti = st[motorIndex].bittiEdge;
  st[motorIndex].bittiEdge = false;
  return bitti;
}

// ═══════════════════════════════════════════════════════════════
// DURDUR
// ═══════════════════════════════════════════════════════════════
void pulseAtDurdur(uint8_t motorIndex) {
  if (motorIndex > 2) return;
  
  // ✅ MOTOR FİZİKSEL DURDURMA
  digitalWrite(enaPins[motorIndex], HIGH);
  
  noInterrupts();
  st[motorIndex].aktif = false;
  st[motorIndex].bittiEdge = false;
  st[motorIndex].sayac = 0;
  st[motorIndex].hedef = 0;
  interrupts();
}

// ═══════════════════════════════════════════════════════════════
// HEPS İNİ DURDUR
// ═══════════════════════════════════════════════════════════════
void pulseAtHepsiniDurdur() {
  for (uint8_t i = 0; i < 3; i++) {
    pulseAtDurdur(i);
  }
}