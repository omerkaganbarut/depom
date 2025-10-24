// MoveTo.cpp - İVMELİ HAREKET MODÜLÜ v4 - ENCODER BAZLI + AKILLI DURDURMA
// ═══════════════════════════════════════════════════════════════
// YENİ ÖZELLİKLER:
// 1. ✅ Encoder bazlı hedef kontrolü (±5 pulse)
// 2. ✅ Segment geçişlerinde ENA LOW kalsın (pozisyon koru)
// 3. ✅ İşlem bittiğinde pulseAtTamamla() çağrısı (enerji kes)
// ═══════════════════════════════════════════════════════════════

#include "MoveTo.h"
#include "PulseAt.h"
#include "Config.h"

// ═══════════════════════════════════════════════════════════════
// PIN TABLOLARI
// ═══════════════════════════════════════════════════════════════
static const uint8_t enaPins[3] = ENA_PINS;

// ═══════════════════════════════════════════════════════════════
// HAREKET FAZI
// ═══════════════════════════════════════════════════════════════
enum MoveFaz {
  FAZ_KAPALI = 0,
  FAZ_HIZLANMA,
  FAZ_SABIT_HIZ,
  FAZ_YAVASLAMA
};

// ═══════════════════════════════════════════════════════════════
// MOTOR DURUMU
// ═══════════════════════════════════════════════════════════════
struct MoveToState {
  bool aktif = false;
  bool bittiEdge = false;
  
  // HEDEF BILGILERI
  long hedefEnc = 0;
  long baslangicEnc = 0;
  unsigned long toplamPulse = 0;
  
  // HAREKET BILGILERI
  MoveFaz faz = FAZ_KAPALI;
  unsigned int maxHz = 0;
  int yon = 0;
  
  // SEGMENT TAKIBI
  unsigned long atilanPulse = 0;
  unsigned long sonSegmentPulse = 0;
  
  // ZAMANLAMA
  unsigned long baslangicMs = 0;
};

static MoveToState st[3];
static StepMotorEncoder* encoders[3] = {nullptr, nullptr, nullptr};

// ═══════════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════════
void moveToSetup(StepMotorEncoder* encZ, 
                 StepMotorEncoder* encX, 
                 StepMotorEncoder* encB) {
  encoders[MOTOR_Z] = encZ;
  encoders[MOTOR_X] = encX;
  encoders[MOTOR_B] = encB;
}

// ═══════════════════════════════════════════════════════════════
// YARDIMCI: ENCODER POZİSYONU
// ═══════════════════════════════════════════════════════════════
static long getEncoderPos(uint8_t motorIndex) {
  if (encoders[motorIndex] == nullptr) {
    return 0;
  }
  return encoders[motorIndex]->getPosition();
}

// ═══════════════════════════════════════════════════════════════
// MOTOR PARAMETRELERİ
// ═══════════════════════════════════════════════════════════════
static unsigned long getRampaPulse(uint8_t motorIndex) {
  switch (motorIndex) {
    case MOTOR_Z:  return ACCEL_RAMP_Z;   // 5000
    case MOTOR_X:  return ACCEL_RAMP_X;   // 5000
    case MOTOR_B:  return ACCEL_RAMP_BIG; // 400
    default:       return 400;
  }
}

static unsigned int getMinSpeed(uint8_t motorIndex) {
  switch (motorIndex) {
    case MOTOR_Z:  return MIN_SPEED_Z;    // 100 Hz
    case MOTOR_X:  return MIN_SPEED_X;    // 100 Hz
    case MOTOR_B:  return MIN_SPEED_BIG;  // 20 Hz
    default:       return 20;
  }
}

// ═══════════════════════════════════════════════════════════════
// HIZ HESAPLAMA
// ═══════════════════════════════════════════════════════════════
static unsigned int hesaplaHiz(MoveToState* s, uint8_t motorIndex) {
  unsigned long rampaPulse = getRampaPulse(motorIndex);
  unsigned int minSpeed = getMinSpeed(motorIndex);
  
  if (s->faz == FAZ_HIZLANMA) {
    double oran = (double)s->atilanPulse / (double)rampaPulse;
    if (oran > 1.0) oran = 1.0;
    
    unsigned int hiz = minSpeed + (unsigned int)((s->maxHz - minSpeed) * oran);
    return hiz;
  }
  else if (s->faz == FAZ_SABIT_HIZ) {
    return s->maxHz;
  }
  else if (s->faz == FAZ_YAVASLAMA) {
    unsigned long kalan = s->toplamPulse - s->atilanPulse;
    double oran = (double)kalan / (double)rampaPulse;
    if (oran > 1.0) oran = 1.0;
    
    unsigned int hiz = minSpeed + (unsigned int)((s->maxHz - minSpeed) * oran);
    return hiz;
  }
  
  return minSpeed;
}

// ═══════════════════════════════════════════════════════════════
// SEGMENT PULSE HESAPLAMA
// ═══════════════════════════════════════════════════════════════
static unsigned long hesaplaSegmentPulse(unsigned int hiz) {
  if (hiz < 10) hiz = 10;
  unsigned long segmentPulse = hiz / 10;
  if (segmentPulse < 1) segmentPulse = 1;
  if (segmentPulse > 100) segmentPulse = 100;
  return segmentPulse;
}

// ═══════════════════════════════════════════════════════════════
// MOVETO BAŞLAT
// ═══════════════════════════════════════════════════════════════
bool moveTo(uint8_t motorIndex, long hedefEnc, unsigned int maxHz) {
  if (motorIndex > 2) {
    return false;
  }
  
  if (encoders[motorIndex] == nullptr) {
    return false;
  }
  
  if (st[motorIndex].aktif) {
    return false;
  }
  
  unsigned int minSpeed = getMinSpeed(motorIndex);
  if (maxHz < minSpeed) {
    return false;
  }
  
  long mevcutEnc = getEncoderPos(motorIndex);
  long fark = hedefEnc - mevcutEnc;
  
  if (fark == 0) {
    st[motorIndex].bittiEdge = true;
    return true;
  }
  
  unsigned long pulse = (unsigned long)abs(fark);
  int yon = (fark > 0) ? 0 : 1;
  
  // ─────────────────────────────────────────────────────────────
  // DURUM BAŞLATMA
  // ─────────────────────────────────────────────────────────────
  st[motorIndex].aktif = true;
  st[motorIndex].bittiEdge = false;
  st[motorIndex].hedefEnc = hedefEnc;
  st[motorIndex].baslangicEnc = mevcutEnc;
  st[motorIndex].maxHz = maxHz;
  st[motorIndex].toplamPulse = pulse;
  st[motorIndex].atilanPulse = 0;
  st[motorIndex].yon = yon;
  st[motorIndex].baslangicMs = millis();
  
  // ─────────────────────────────────────────────────────────────
  // FAZ BELİRLEME
  // ─────────────────────────────────────────────────────────────
  unsigned long rampaPulse = getRampaPulse(motorIndex);
  
  if (pulse <= rampaPulse) {
    st[motorIndex].faz = FAZ_YAVASLAMA;
  } 
  else if (pulse <= 2 * rampaPulse) {
    st[motorIndex].faz = FAZ_HIZLANMA;
  } 
  else {
    st[motorIndex].faz = FAZ_HIZLANMA;
  }
  
  // ─────────────────────────────────────────────────────────────
  // İLK SEGMENTİ BAŞLAT
  // ─────────────────────────────────────────────────────────────
  unsigned int hiz = hesaplaHiz(&st[motorIndex], motorIndex);
  unsigned long segmentPulse = hesaplaSegmentPulse(hiz);
  
  if (segmentPulse > pulse) {
    segmentPulse = pulse;
  }
  
  st[motorIndex].sonSegmentPulse = segmentPulse;
  
  useMotor(motorIndex);
  pulseAt(segmentPulse, yon, hiz);
  
  return true;
}

// ═══════════════════════════════════════════════════════════════
// MOVETO ARKA PLAN - ENCODER BAZLI KESİN HEDEFLEME
// ═══════════════════════════════════════════════════════════════
void moveToRun() {
  for (uint8_t m = 0; m < 3; m++) {
    if (!st[m].aktif) {
      continue;
    }
    
    // ───────────────────────────────────────────────────────────
    // pulseAt arka planını çalıştır
    // ───────────────────────────────────────────────────────────
    useMotor(m);
    pulseAt(0, 0, 0);
    
    // ───────────────────────────────────────────────────────────
    // Segment bitti mi?
    // ───────────────────────────────────────────────────────────
    if (pulseAtBittiMi(m)) {
      
      // ─────────────────────────────────────────────────────────
      // ✅ ENCODER BAZLI HEDEF KONTROLÜ
      // ─────────────────────────────────────────────────────────
      long mevcutEnc = getEncoderPos(m);
      long kalan = st[m].hedefEnc - mevcutEnc;
      long kalanAbs = abs(kalan);
      
      // ─────────────────────────────────────────────────────────
      // HEDEFE ULAŞILDI MI? (±5 pulse tolerans)
      // ─────────────────────────────────────────────────────────
      if (kalanAbs <= 3) {
        // ✅ HEDEF TAMAM
        st[m].aktif = false;
        st[m].bittiEdge = true;
        st[m].faz = FAZ_KAPALI;
        st[m].atilanPulse = 0;
        st[m].toplamPulse = 0;
        
        // ✅ MOTOR DİNLENDİR (ENA HIGH - enerji tasarrufu)
        // NOT: MoveTo tamamlandığında motor dinlensin
        pulseAtTamamla(m);
        
        // BİLGİ: Encoder farkı (varsa)
        if (kalanAbs > 0) {
          Serial.print(F("[MoveTo] M"));
          Serial.print(m);
          Serial.print(F(" → Hedef: "));
          Serial.print(st[m].hedefEnc);
          Serial.print(F(" | Mevcut: "));
          Serial.print(mevcutEnc);
          Serial.print(F(" | Fark: "));
          Serial.print(kalan);
          Serial.println(F("p ✓"));
        }
        
        continue;
      }
      
      // ─────────────────────────────────────────────────────────
      // HEDEF AŞILDI MI? (GÜVENLİK)
      // ─────────────────────────────────────────────────────────
      st[m].atilanPulse += st[m].sonSegmentPulse;
      
      if (st[m].atilanPulse > st[m].toplamPulse + 1000) {
        Serial.print(F("[MoveTo] M"));
        Serial.print(m);
        Serial.print(F(" UYARI: Hedef aşıldı! Kalan="));
        Serial.print(kalan);
        Serial.println(F("p - Durduruluyor."));
        
        st[m].aktif = false;
        st[m].bittiEdge = true;
        st[m].faz = FAZ_KAPALI;
        
        pulseAtTamamla(m);  // ✅ Motor dinlendir
        continue;
      }
      
      // ─────────────────────────────────────────────────────────
      // FAZ GEÇİŞLERİ
      // ─────────────────────────────────────────────────────────
      unsigned long rampaPulse = getRampaPulse(m);
      
      if (st[m].faz == FAZ_HIZLANMA) {
        if (st[m].atilanPulse >= rampaPulse) {
          if (kalanAbs > (long)rampaPulse) {
            st[m].faz = FAZ_SABIT_HIZ;
          } else {
            st[m].faz = FAZ_YAVASLAMA;
          }
        }
      }
      else if (st[m].faz == FAZ_SABIT_HIZ) {
        if (kalanAbs <= (long)rampaPulse) {
          st[m].faz = FAZ_YAVASLAMA;
        }
      }
      
      // ─────────────────────────────────────────────────────────
      // SONRAKİ SEGMENT: ENCODER'A GÖRE HESAPLA
      // ─────────────────────────────────────────────────────────
      unsigned int hiz = hesaplaHiz(&st[m], m);
      unsigned long segmentPulse = hesaplaSegmentPulse(hiz);
      
      if (segmentPulse > (unsigned long)kalanAbs) {
        segmentPulse = (unsigned long)kalanAbs;
      }
      
      // Yön düzeltmesi
      int yon = (kalan > 0) ? 0 : 1;
      
      st[m].sonSegmentPulse = segmentPulse;
      
      useMotor(m);
      pulseAt(segmentPulse, yon, hiz);
    }
  }
}

// ═══════════════════════════════════════════════════════════════
// DURUM FONKSİYONLARI
// ═══════════════════════════════════════════════════════════════
bool moveToBittiMi(uint8_t motorIndex) {
  if (motorIndex > 2) {
    return false;
  }
  
  if (st[motorIndex].bittiEdge) {
    st[motorIndex].bittiEdge = false;
    return true;
  }
  return false;
}

bool moveToAktifMi(uint8_t motorIndex) {
  if (motorIndex > 2) {
    return false;
  }
  return st[motorIndex].aktif;
}

long moveToKalan(uint8_t motorIndex) {
  if (motorIndex > 2) {
    return 0;
  }
  
  if (encoders[motorIndex] != nullptr) {
    long mevcutEnc = encoders[motorIndex]->getPosition();
    return abs(st[motorIndex].hedefEnc - mevcutEnc);
  }
  
  return (long)(st[motorIndex].toplamPulse - st[motorIndex].atilanPulse);
}

// ═══════════════════════════════════════════════════════════════
// DURDURMA FONKSİYONLARI
// ═══════════════════════════════════════════════════════════════
void moveToDurdur(uint8_t motorIndex) {
  if (motorIndex > 2) {
    return;
  }
  
  // ✅ DURUMU TEMİZLE
  st[motorIndex].aktif = false;
  st[motorIndex].bittiEdge = false;
  st[motorIndex].faz = FAZ_KAPALI;
  st[motorIndex].atilanPulse = 0;
  st[motorIndex].toplamPulse = 0;
  
  // ✅ ACİL DURDURMA (kullanıcı komutuysa)
  pulseAtAcilDurdur(motorIndex);
}

void moveToHepsiniDurdur() {
  for (uint8_t m = 0; m < 3; m++) {
    moveToDurdur(m);
  }
}