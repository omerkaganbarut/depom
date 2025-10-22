// OynatmaModulu.cpp - v2.4 FINAL
// ✅ X motor → Z motor değişimi
// ✅ Dinamik mapping parametreleri
// ✅ Serial.print sadeleştirildi
// ✅ Derleme hatası düzeltildi

#include "OynatmaModulu.h"
#include "Config.h"
#include "KayitModulu.h"
#include "PulseAt.h"
#include "stepmotorenkoderiokuma.h"
#include "A0Filtre.h"

// ═══════════════════════════════════════════════════════════════
// DURUM MAKİNESİ
// ═══════════════════════════════════════════════════════════════
enum OynatmaDurum {
  OYNATMA_KAPALI = 0,
  OYNATMA_HIZALAMA,
  OYNATMA_HIZALAMA_BEKLE,
  OYNATMA_SEGMENT_HESAPLA,
  OYNATMA_SEGMENT_MOTOR,
  OYNATMA_TAMAMLANDI
};

// ═══════════════════════════════════════════════════════════════
// STATİK DEĞİŞKENLER
// ═══════════════════════════════════════════════════════════════
static uint16_t idx = 0;
static OynatmaDurum durum = OYNATMA_KAPALI;

// Encoder pointer'ları
static StepMotorEncoder* bigEnc = nullptr;
static StepMotorEncoder* zEnc = nullptr;

// A0 aralığı
static uint16_t a0Min = 1023;
static uint16_t a0Max = 0;

// DİNAMİK PARAMETRELER (main.cpp'den alınır)
static long* bigFreqMinPtr = nullptr;
static long* bigFreqMaxPtr = nullptr;
static long* zEncMinPtr = nullptr;
static long* zEncMaxPtr = nullptr;

// ═══════════════════════════════════════════════════════════════
// GÜVENLİ MAP (64-bit overflow korumalı)
// ═══════════════════════════════════════════════════════════════
long mapSafe(long x, long in_min, long in_max, long out_min, long out_max) {
  if (in_max == in_min) {
    return out_min;
  }
  int64_t num = (int64_t)(x - in_min) * (int64_t)(out_max - out_min);
  int64_t den = (int64_t)(in_max - in_min);
  return (long)(num / den) + out_min;
}

// ═══════════════════════════════════════════════════════════════
// ENCODER SETUP
// ═══════════════════════════════════════════════════════════════
void oynatmaEncoderSetup(StepMotorEncoder* bigEncoder, StepMotorEncoder* zEncoder) {
  bigEnc = bigEncoder;
  zEnc = zEncoder;
}

// ═══════════════════════════════════════════════════════════════
// PARAMETRE POINTER SETUP
// ═══════════════════════════════════════════════════════════════
void oynatmaParametreSetup(long* bigFreqMin, long* bigFreqMax, long* zEncMin, long* zEncMax) {
  bigFreqMinPtr = bigFreqMin;
  bigFreqMaxPtr = bigFreqMax;
  zEncMinPtr = zEncMin;
  zEncMaxPtr = zEncMax;
}

// ═══════════════════════════════════════════════════════════════
// OYNATMA BAŞLATMA (GERÇEK)
// ═══════════════════════════════════════════════════════════════
void oynatmaBaslatGercek() {
  // Kontroller
  if (!kayitTamamlandiMi()) {
    Serial.println(F("✗ Kayıt yok!"));
    return;
  }
  
  const KM_Sample* samples = kayitVerileri();
  
  // A0 min/max hesapla
  a0Min = 1023;
  a0Max = 0;
  for (uint16_t i = 0; i < kayitOrnekSayisi(); i++) {
    if (samples[i].a0 < a0Min) a0Min = samples[i].a0;
    if (samples[i].a0 > a0Max) a0Max = samples[i].a0;
  }
  
  Serial.print(F("  A0 Aralık: "));
  Serial.print(a0Min);
  Serial.print(F(" - "));
  Serial.println(a0Max);
  
  // Encoder kontrol
  if (bigEnc == nullptr || zEnc == nullptr) {
    Serial.println(F("✗ Encoder hatası!"));
    return;
  }
  
  // Parametre pointer kontrolü
  if (bigFreqMinPtr == nullptr || bigFreqMaxPtr == nullptr || 
      zEncMinPtr == nullptr || zEncMaxPtr == nullptr) {
    Serial.println(F("✗ Parametre hatası!"));
    return;
  }
  
  // Hizalama hesapla
  long bigNow = bigEnc->getPosition();
  long bigHedef = samples[0].enc;
  long bigFark = bigHedef - bigNow;
  unsigned long bigPulse = (unsigned long)abs(bigFark);
  int bigYon = (bigFark > 0) ? 0 : 1;
  
  long zNow = zEnc->getPosition();
  long zHedef = mapSafe(samples[0].a0, a0Min, a0Max, *zEncMinPtr, *zEncMaxPtr);
  long zFark = zHedef - zNow;
  unsigned long zPulse = (unsigned long)abs(zFark);
  int zYon = (zFark > 0) ? 0 : 1;
  
  Serial.print(F("  BIG: "));
  Serial.print(bigNow);
  Serial.print(F(" → "));
  Serial.println(bigHedef);
  
  Serial.print(F("  Z  : "));
  Serial.print(zNow);
  Serial.print(F(" → "));
  Serial.println(zHedef);
  
  // Motorları başlat
  if (bigPulse > 0) {
    useMotor(MOTOR_B);
    pulseAt(bigPulse, bigYon, 100);
  }
  
  if (zPulse > 0) {
    useMotor(MOTOR_Z);
    pulseAt(zPulse, zYon, 2000);
  }
  
  idx = 0;
  durum = OYNATMA_HIZALAMA_BEKLE;
  Serial.println(F("✓ Başlatıldı\n"));
}

// ═══════════════════════════════════════════════════════════════
// OYNATMA ARKA PLAN
// ═══════════════════════════════════════════════════════════════
void oynatmaRun() {
  const KM_Sample* samples = kayitVerileri();
  
  int a0Start, a0End;
  unsigned int masterFreq, slaveFreq;
  long bigTarget, bigNow, dBig;
  unsigned long masterPulses, slavePulses;
  int masterYon, slaveYon;
  long slaveHedef, zNow, dZ;
  double fsd;
  long fsi;
  
  switch (durum) {
    
    case OYNATMA_KAPALI:
      return;
    
    case OYNATMA_HIZALAMA:
      durum = OYNATMA_HIZALAMA_BEKLE;
      break;
    
    case OYNATMA_HIZALAMA_BEKLE:
      // BIG motor arka planı
      if (pulseAtAktifMi(MOTOR_B)) {
        useMotor(MOTOR_B);
        pulseAt(0, 0, 0);
      }
      
      // Z motor arka planı
      if (pulseAtAktifMi(MOTOR_Z)) {
        useMotor(MOTOR_Z);
        pulseAt(0, 0, 0);
      }
      
      // Her ikisi de bitti mi?
      if (!pulseAtAktifMi(MOTOR_B) && !pulseAtAktifMi(MOTOR_Z)) {
        Serial.print(F("✓ Hizalama → BIG: "));
        Serial.print(bigEnc->getPosition());
        Serial.print(F(" | Z: "));
        Serial.println(zEnc->getPosition());
        
        idx = 1;
        durum = OYNATMA_SEGMENT_HESAPLA;
      }
      break;
    
    case OYNATMA_SEGMENT_HESAPLA:
      Serial.print(F("\n[SEG "));
      Serial.print(idx - 1);
      Serial.print(F("→"));
      Serial.print(idx);
      Serial.println(F("]"));
      
      // BIG MOTOR (MASTER)
      a0Start = samples[idx - 1].a0;
      masterFreq = (unsigned int)mapSafe(a0Start, a0Min, a0Max, *bigFreqMinPtr, *bigFreqMaxPtr);
      
      bigTarget = samples[idx].enc;
      bigNow = bigEnc->getPosition();
      dBig = bigTarget - bigNow;
      masterPulses = (unsigned long)abs(dBig);
      masterYon = (dBig > 0) ? 0 : 1;
      
      Serial.print(F("  BIG: "));
      Serial.print(bigNow);
      Serial.print(F("→"));
      Serial.print(bigTarget);
      Serial.print(F(" ("));
      Serial.print(masterFreq);
      Serial.println(F("Hz)"));
      
      // Z MOTOR (SLAVE)
      a0End = samples[idx].a0;
      slaveHedef = mapSafe(a0End, a0Min, a0Max, *zEncMinPtr, *zEncMaxPtr);
      
      zNow = zEnc->getPosition();
      dZ = slaveHedef - zNow;
      slavePulses = (unsigned long)abs(dZ);
      slaveYon = (dZ > 0) ? 0 : 1;
      
      Serial.print(F("  Z  : "));
      Serial.print(zNow);
      Serial.print(F("→"));
      Serial.print(slaveHedef);
      Serial.print(F(" ("));
      
      // Senkron hız hesapla
      slaveFreq = 0;
      if (masterPulses > 0 && slavePulses > 0) {
        fsd = (double)slavePulses * (double)masterFreq / (double)masterPulses;
        fsi = lround(fsd);
        if (fsi < 1) fsi = 1;
        slaveFreq = (unsigned int)fsi;
        Serial.print(slaveFreq);
      } else {
        Serial.print(F("0"));
      }
      Serial.println(F("Hz)"));
      
      // Motorları başlat
      if (masterPulses > 0 && masterFreq > 0) {
        useMotor(MOTOR_B);
        pulseAt(masterPulses, masterYon, masterFreq);
      }
      
      if (slavePulses > 0 && slaveFreq > 0) {
        useMotor(MOTOR_Z);
        pulseAt(slavePulses, slaveYon, slaveFreq);
      }
      
      durum = OYNATMA_SEGMENT_MOTOR;
      break;
    
    case OYNATMA_SEGMENT_MOTOR:
      // BIG motor arka planı
      if (pulseAtAktifMi(MOTOR_B)) {
        useMotor(MOTOR_B);
        pulseAt(0, 0, 0);
      }
      
      // Z motor arka planı
      if (pulseAtAktifMi(MOTOR_Z)) {
        useMotor(MOTOR_Z);
        pulseAt(0, 0, 0);
      }
      
      // Her ikisi de bitti mi?
      if (!pulseAtAktifMi(MOTOR_B) && !pulseAtAktifMi(MOTOR_Z)) {
        // Varış noktalarını göster
        Serial.print(F("  ✓ BIG: "));
        Serial.print(bigEnc->getPosition());
        Serial.print(F(" | Z: "));
        Serial.println(zEnc->getPosition());
        
        // Son segment mi?
        if (idx >= kayitOrnekSayisi() - 1) {
          durum = OYNATMA_TAMAMLANDI;
          Serial.println(F("\n[OYNATMA] Tamamlandı! ✓\n"));
        } else {
          idx++;
          durum = OYNATMA_SEGMENT_HESAPLA;
        }
      }
      break;
    
    case OYNATMA_TAMAMLANDI:
      return;
    
    default:
      Serial.println(F("✗ Bilinmeyen durum!"));
      durum = OYNATMA_KAPALI;
      break;
  }
}

// ═══════════════════════════════════════════════════════════════
// OYNATMA AKTİF Mİ
// ═══════════════════════════════════════════════════════════════
bool oynatmaAktifMi() {
  return (durum != OYNATMA_KAPALI && durum != OYNATMA_TAMAMLANDI);
}

// ═══════════════════════════════════════════════════════════════
// OYNATMA TAMAMLANDI MI
// ═══════════════════════════════════════════════════════════════
bool oynatmaTamamlandiMi() {
  return (durum == OYNATMA_TAMAMLANDI);
}

// ═══════════════════════════════════════════════════════════════
// SEGMENT INDEX
// ═══════════════════════════════════════════════════════════════
uint16_t oynatmaSegmentIndex() {
  return idx;
}

// ═══════════════════════════════════════════════════════════════
// OYNATMA DURDUR
// ═══════════════════════════════════════════════════════════════
void oynatmaDurdur() {
  Serial.println(F("\n[OYNATMA] Acil durduruldu!"));
  
  pulseAtDurdur(MOTOR_B);
  pulseAtDurdur(MOTOR_Z);
  
  durum = OYNATMA_KAPALI;
  
  Serial.print(F("   Segment: "));
  Serial.println(idx);
  Serial.println();
}