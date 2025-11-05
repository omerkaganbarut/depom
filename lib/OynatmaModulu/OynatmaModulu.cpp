// OynatmaModulu.cpp - v6.0 GLOBAL A0 ARALIĞI KULLANIMI
// ═══════════════════════════════════════════════════════════════
// GÖREV: Verilen bir kayıt listesini BIG+Z ile oynatmak
// ✅ Global A0 aralığı kullanımı (CiftKayitModulu'nden)
// ✅ Z sıfırlama globalA0Min'de yapıldığı için local hesap yok
// ═══════════════════════════════════════════════════════════════

#include "OynatmaModulu.h"
#include "Config.h"
#include "CiftKayitModulu.h"
#include "PulseAt.h"
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════
// DURUM MAKİNESİ
// ═══════════════════════════════════════════════════════════════
enum OynatmaDurum {
  OY_KAPALI = 0,
  OY_SEGMENT_OYNAT,
  OY_TAMAMLANDI
};

// ═══════════════════════════════════════════════════════════════
// STATİK DEĞİŞKENLER
// ═══════════════════════════════════════════════════════════════
static OynatmaDurum durum = OY_KAPALI;
static uint16_t idx = 0;


// Encoder pointer'ları
static StepMotorEncoder* bigEnc = nullptr;
static StepMotorEncoder* zEnc = nullptr;

// Parametre pointer'ları
static long* bigFreqMinPtr = nullptr;
static long* bigFreqMaxPtr = nullptr;
static long* bigFreqRefPtr = nullptr; 
// ✅ KAYIT POINTER (kayit1 veya kayit2 olabilir)
static const CK_Sample* kayitPtr = nullptr;
static uint16_t kayitOrnekSayisi = 0;

// ═══════════════════════════════════════════════════════════════
// ✅ GLOBAL A0 ARALIĞINI
// KULLAN (CiftKayitModulu'nden)
// ═══════════════════════════════════════════════════════════════
extern uint16_t globalA0Min;
extern uint16_t globalA0Max;

// ═══════════════════════════════════════════════════════════════
// HELPER: GLOBAL A0 ARALIĞINA GÖRE Z MAX HESAPLA
// ═══════════════════════════════════════════════════════════════
static inline long hesaplaZMax() {
  // Global A0 ARALIĞININ fiziksel Z karşılığı
  // Örnek: globalA0Min=300, globalA0Max=550
  //        Aralık = 550-300 = 250
  //        Z Max = (250/1023) × 160000 = 39,100
  
  if (globalA0Max <= globalA0Min) return Z_ENCODER_MAX;
  
  // A0 aralığı
  uint16_t a0Aralik = globalA0Max - globalA0Min;
  
  // A0 aralık oranı × Z_ENCODER_MAX
  float oran = (float)a0Aralik / 1023.0;
  long zMax = (long)(oran * Z_ENCODER_MAX);
  
  // Güvenlik kontrolü
  if (zMax > Z_ENCODER_MAX) zMax = Z_ENCODER_MAX;
  if (zMax < 1000) zMax = 1000;  // Minimum 1000 encoder
  
  return zMax;
}

// ═══════════════════════════════════════════════════════════════
// REFERANS HIZ SETUP
// ═══════════════════════════════════════════════════════════════
void oynatmaRefHizSetup(long* bigFreqRefPtrArg) {
  bigFreqRefPtr = bigFreqRefPtrArg;
}
// ═══════════════════════════════════════════════════════════════
// HELPER: A0 → ZEnc MAPPING (GLOBAL ARALIĞA GÖRE)
// ═══════════════════════════════════════════════════════════════
static inline long mapA0ToZEnc(uint16_t a0) {
  if (a0 <= globalA0Min) return 0;
  
  // ✅ Dinamik Z max hesapla
  long zMax = hesaplaZMax();
  
  if (a0 >= globalA0Max) return zMax;
  
  // globalA0Min → 0, globalA0Max → zMax
  return map(a0, globalA0Min, globalA0Max, 0, zMax);
}

// ═══════════════════════════════════════════════════════════════
// HELPER: A0 → BigFreq MAPPING (Ters orantılı)
// ═══════════════════════════════════════════════════════════════
static inline unsigned int mapA0ToBigFreq(uint16_t a0) {
  if (bigFreqMinPtr == nullptr || bigFreqMaxPtr == nullptr) return 100;
  if (bigFreqRefPtr == nullptr) return 100;  // ✅ YENİ KONTROL
  
  // A0 → mm DÖNÜŞÜM KATSAYISI
  float mmPerA0 = A0_FIZIKSEL_ARALIK_MM / 1023.0;
  
  // REFERANS YARICAP (Depo kenarı çapının yarısı)
  float yaricapRef = DEPO_KENAR_CAP_MM / 2.0;
  
  // A0 DEĞİŞİMİNDEN KAYNAKLANAN YARICAP DEĞİŞİMİ
  float deltaYaricapMM = (a0 - globalA0Min) * mmPerA0;
  
  // GÜNCEL YARICAP
  float yaricapMM = yaricapRef + deltaYaricapMM;
  
  // TERS ORANTILI HIZ HESABI
  float sabitCarpim = yaricapRef * (*bigFreqRefPtr);  // ✅ POINTER KULLAN
  float freq = sabitCarpim / yaricapMM;
  
  // ALT SINIR KONTROLÜ
  if (freq < 10) freq = 10;
  
  return (unsigned int)freq;
}
// ═══════════════════════════════════════════════════════════════
// ENCODER SETUP
// ═══════════════════════════════════════════════════════════════
void oynatmaEncoderSetup(StepMotorEncoder* bigEncoder, StepMotorEncoder* zEncoder) {
  bigEnc = bigEncoder;
  zEnc = zEncoder;
}

// ═══════════════════════════════════════════════════════════════
// PARAMETRE SETUP
// ═══════════════════════════════════════════════════════════════
void oynatmaParametreSetup(long* bigFreqMin, long* bigFreqMax, 
                           long* zEncMin, long* zEncMax) {
  bigFreqMinPtr = bigFreqMin;
  bigFreqMaxPtr = bigFreqMax;
  
  // NOT: zEncMin/Max fiziksel 0-160000 kullanıldığı için kullanılmıyor
  (void)zEncMin;
  (void)zEncMax;
}

// ═══════════════════════════════════════════════════════════════
// ✅ KAYIT BAZLI OYNATMA BAŞLATMA
// ═══════════════════════════════════════════════════════════════
void oynatmaBaslatKayit(const CK_Sample* kayit, uint16_t ornekSayisi) {
  Serial.println(F("\n[OYNATMA] Başlatılıyor..."));
  
  // ─────────────────────────────────────────────────────────────
  // KONTROLLER
  // ─────────────────────────────────────────────────────────────
  if (kayit == nullptr) {
    Serial.println(F("✗ Kayıt pointer hatası!"));
    return;
  }
  
  if (ornekSayisi == 0) {
    Serial.println(F("✗ Örnek sayısı sıfır!"));
    return;
  }
  
  if (bigEnc == nullptr || zEnc == nullptr) {
    Serial.println(F("✗ Encoder hatası!"));
    return;
  }
  
  if (bigFreqMinPtr == nullptr || bigFreqMaxPtr == nullptr) {
    Serial.println(F("✗ Parametre hatası!"));
    return;
  }
  
  // ─────────────────────────────────────────────────────────────
  // KAYIT POINTER'INI SAKLA
  // ─────────────────────────────────────────────────────────────
  kayitPtr = kayit;
  kayitOrnekSayisi = ornekSayisi;
  
  // ─────────────────────────────────────────────────────────────
  // ✅ GLOBAL A0 BİLGİLERİNİ GÖSTER
  // ─────────────────────────────────────────────────────────────
  Serial.print(F("  Global A0 Min: "));
  Serial.println(globalA0Min);

  Serial.print(F("  Global A0 Max: "));
  Serial.println(globalA0Max);
  
  Serial.print(F("  A0 Aralığı: "));
  Serial.println(globalA0Max - globalA0Min);
  
  Serial.print(F("  Hesaplanan Z Max: "));
  Serial.println(hesaplaZMax());
  
  Serial.print(F("  Örnek sayısı: "));
  Serial.println(ornekSayisi);
  
  // ─────────────────────────────────────────────────────────────
  // BAŞLAT
  // ─────────────────────────────────────────────────────────────
  idx = 0;
  durum = OY_SEGMENT_OYNAT;
  digitalWrite(KAYNAK_ROLE_PIN, LOW); // KAYNAK AÇ
  
  Serial.println(F("[OYNATMA] Segment oynatma başladı!"));
}

// ═══════════════════════════════════════════════════════════════
// OYNATMA ARKA PLAN
// ═══════════════════════════════════════════════════════════════
void oynatmaRun() {
  if (durum == OY_KAPALI || durum == OY_TAMAMLANDI) return;
  if (kayitPtr == nullptr) return;
  
  switch (durum) {
    
    // ───────────────────────────────────────────────────────────
    case OY_SEGMENT_OYNAT:
    {
      if (!pulseAtAktifMi(MOTOR_B) && !pulseAtAktifMi(MOTOR_Z)) {
        
        // Son segment mi?
        if (idx >= kayitOrnekSayisi - 1) {
          Serial.println(F("[OYNATMA] ✓ Tamamlandı!"));
          digitalWrite(KAYNAK_ROLE_PIN, HIGH); // KAYNAK KAPAT
          durum = OY_TAMAMLANDI;
          return;
        }
        
        // ✅ SEGMENT HESAPLA: idx → idx+1
        uint16_t a0Start = kayitPtr[idx].a0;
        uint16_t a0End = kayitPtr[idx + 1].a0;
        
        // BIG MOTOR
        long bigTarget = kayitPtr[idx + 1].enc;
        long bigNow = bigEnc->getPosition();
        long dBig = bigTarget - bigNow;
        unsigned long masterPulses = (unsigned long)abs(dBig);
        int masterYon = (dBig > 0) ? 0 : 1;
        unsigned int masterFreq = mapA0ToBigFreq(a0Start);
        
        // Z MOTOR
        long zHedef = mapA0ToZEnc(a0End);
        long zNow = zEnc->getPosition();
        long dZ = zHedef - zNow;
        unsigned long slavePulses = (unsigned long)abs(dZ);
        int slaveYon = (dZ > 0) ? 0 : 1;
        
        // Senkron hız hesapla
        unsigned int slaveFreq = 0;
        if (masterPulses > 0 && slavePulses > 0) {
          double fsd = (double)slavePulses * (double)masterFreq / (double)masterPulses;
          slaveFreq = (unsigned int)lround(fsd);
          if (slaveFreq < 1) slaveFreq = 1;
        }
        
        // ═══════════════════════════════════════════════════════════
        // 📊 DEBUG: SEGMENT BİLGİLERİ
        // ═══════════════════════════════════════════════════════════
        Serial.print(F("  [SEG ")); Serial.print(idx); 
        Serial.print(F("→")); Serial.print(idx + 1);
        Serial.print(F("] A0: ")); Serial.print(a0Start);
        Serial.print(F("→")); Serial.print(a0End);
        Serial.print(F(" | BIG: ")); Serial.print(bigNow);
        Serial.print(F("→")); Serial.print(bigTarget);
        Serial.print(F(" (Δ=")); Serial.print(dBig);
        Serial.print(F(", F=")); Serial.print(masterFreq); Serial.print(F("Hz)"));
        Serial.print(F(" | Z: ")); Serial.print(zNow);
        Serial.print(F("→")); Serial.print(zHedef);
        Serial.print(F(" (Δ=")); Serial.print(dZ);
        Serial.print(F(", F=")); Serial.print(slaveFreq); Serial.println(F("Hz)"));
        
        // MOTORLARI BAŞLAT
        if (masterPulses > 0 && masterFreq > 0) {
          useMotor(MOTOR_B);
          pulseAt(masterPulses, masterYon, masterFreq);
        }
        
        if (slavePulses > 0 && slaveFreq > 0) {
          useMotor(MOTOR_Z);
          pulseAt(slavePulses, slaveYon, slaveFreq);
        }
        
        idx++;
      }
      
      // Arka plan
      if (pulseAtAktifMi(MOTOR_B)) {
        useMotor(MOTOR_B);
        pulseAt(0, 0, 0);
      }
      
      if (pulseAtAktifMi(MOTOR_Z)) {
        useMotor(MOTOR_Z);
        pulseAt(0, 0, 0);
      }
    }
    break;
    
    default:
      break;
  }
}

// ═══════════════════════════════════════════════════════════════
// DURUM FONKSİYONLARI
// ═══════════════════════════════════════════════════════════════
bool oynatmaAktifMi() {
  return (durum != OY_KAPALI && durum != OY_TAMAMLANDI);
}

bool oynatmaTamamlandiMi() {
  return (durum == OY_TAMAMLANDI);
}

uint16_t oynatmaSegmentIndex() {
  return idx;
}

// ═══════════════════════════════════════════════════════════════
// DURDUR
// ═══════════════════════════════════════════════════════════════
void oynatmaDurdur() {
  Serial.println(F("[OYNATMA] Durduruldu!"));
  
  pulseAtDurdur(MOTOR_B);
  pulseAtDurdur(MOTOR_Z);
  
  durum = OY_KAPALI;
  kayitPtr = nullptr;
}