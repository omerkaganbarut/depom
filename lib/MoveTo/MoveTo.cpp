// MoveTo.cpp - İVMELİ HAREKET MODÜLÜ v2 FINAL
#include "MoveTo.h"
#include "PulseAt.h"
#include "Config.h"  // ← ENA_PINS için eklendi

// ═══════════════════════════════════════════════════════════════
// PIN TABLOLARI (Config.h'den)
// ═══════════════════════════════════════════════════════════════
static const uint8_t enaPins[3] = ENA_PINS;  // ENABLE pin dizisi

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
  long hedefEnc = 0;
  long baslangicEnc = 0;
  
  MoveFaz faz = FAZ_KAPALI;
  unsigned int maxHz = 0;
  unsigned long toplamPulse = 0;
  unsigned long kalanPulse = 0;
  int yon = 0;
  
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
// HIZ HESAPLAMA
// ═══════════════════════════════════════════════════════════════
static unsigned int hesaplaHiz(MoveToState* s) {
  if (s->faz == FAZ_HIZLANMA) {
    unsigned long tamamlanan = s->toplamPulse - s->kalanPulse;
    double oran = (double)tamamlanan / (double)ACCEL_RAMP_PULSE;
    if (oran > 1.0) oran = 1.0;
    unsigned int hiz = MIN_SPEED_HZ + (unsigned int)((s->maxHz - MIN_SPEED_HZ) * oran);
    return hiz;
  }
  else if (s->faz == FAZ_SABIT_HIZ) {
    return s->maxHz;
  }
  else if (s->faz == FAZ_YAVASLAMA) {
    double oran = (double)s->kalanPulse / (double)ACCEL_RAMP_PULSE;
    if (oran > 1.0) oran = 1.0;
    unsigned int hiz = MIN_SPEED_HZ + (unsigned int)((s->maxHz - MIN_SPEED_HZ) * oran);
    return hiz;
  }
  return MIN_SPEED_HZ;
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
// MOVETO - İVMELİ HAREKET BAŞLAT
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
  
  if (maxHz < MIN_SPEED_HZ) {
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
  
  st[motorIndex].aktif = true;
  st[motorIndex].bittiEdge = false;
  st[motorIndex].hedefEnc = hedefEnc;
  st[motorIndex].baslangicEnc = mevcutEnc;
  st[motorIndex].maxHz = maxHz;
  st[motorIndex].toplamPulse = pulse;
  st[motorIndex].kalanPulse = pulse;
  st[motorIndex].yon = yon;
  st[motorIndex].baslangicMs = millis();
  
  if (pulse <= ACCEL_RAMP_PULSE) {
    st[motorIndex].faz = FAZ_YAVASLAMA;
  } 
  else if (pulse <= 2 * ACCEL_RAMP_PULSE) {
    st[motorIndex].faz = FAZ_HIZLANMA;
  } 
  else {
    st[motorIndex].faz = FAZ_HIZLANMA;
  }
  
  unsigned int hiz = hesaplaHiz(&st[motorIndex]);
  unsigned long segmentPulse = hesaplaSegmentPulse(hiz);
  
  if (segmentPulse > st[motorIndex].kalanPulse) {
    segmentPulse = st[motorIndex].kalanPulse;
  }
  
  useMotor(motorIndex);
  pulseAt(segmentPulse, yon, hiz);
  
  return true;
}

// ═══════════════════════════════════════════════════════════════
// MOVETO ARKA PLAN
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
      // ✅ KALAN PULSE'U GÜNCELLE (Encoder'dan gerçek fark)
      // ─────────────────────────────────────────────────────────
      long mevcutEnc = getEncoderPos(m);
      long gercekFark = st[m].hedefEnc - mevcutEnc;
      st[m].kalanPulse = (unsigned long)abs(gercekFark);
      
      // ─────────────────────────────────────────────────────────
      // ✅ HEDEFE ULAŞILDI MI? (TOLERANS: ±5 pulse)
      // ─────────────────────────────────────────────────────────
      if (st[m].kalanPulse <= 5) {
        // ✅ HEDEFE ULAŞILDI!
        st[m].aktif = false;
        st[m].bittiEdge = true;
        st[m].faz = FAZ_KAPALI;
        
        // ✅ MOTORU FİZİKSEL OLARAK DURDUR
        digitalWrite(enaPins[m], HIGH);
        
        continue;
      }
      
      // ─────────────────────────────────────────────────────────
      // FAZ GEÇİŞLERİ
      // ─────────────────────────────────────────────────────────
      unsigned long tamamlanan = st[m].toplamPulse - st[m].kalanPulse;
      
      if (st[m].faz == FAZ_HIZLANMA) {
        if (tamamlanan >= ACCEL_RAMP_PULSE) {
          if (st[m].kalanPulse > ACCEL_RAMP_PULSE) {
            st[m].faz = FAZ_SABIT_HIZ;
          } else {
            st[m].faz = FAZ_YAVASLAMA;
          }
        }
      }
      else if (st[m].faz == FAZ_SABIT_HIZ) {
        if (st[m].kalanPulse <= ACCEL_RAMP_PULSE) {
          st[m].faz = FAZ_YAVASLAMA;
        }
      }
      
      // ─────────────────────────────────────────────────────────
      // SONRAKİ SEGMENTİ BAŞLAT
      // ─────────────────────────────────────────────────────────
      unsigned int hiz = hesaplaHiz(&st[m]);
      unsigned long segmentPulse = hesaplaSegmentPulse(hiz);
      
      if (segmentPulse > st[m].kalanPulse) {
        segmentPulse = st[m].kalanPulse;
      }
      
      useMotor(m);
      pulseAt(segmentPulse, st[m].yon, hiz);
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
  return (long)st[motorIndex].kalanPulse;
}

// ═══════════════════════════════════════════════════════════════
// DURDURMA FONKSİYONLARI
// ═══════════════════════════════════════════════════════════════
void moveToDurdur(uint8_t motorIndex) {
  if (motorIndex > 2) {
    return;
  }
  
  // ✅ MOTORU FİZİKSEL OLARAK DURDUR
  digitalWrite(enaPins[motorIndex], HIGH);
  
  // pulseAt'ı durdur
  pulseAtDurdur(motorIndex);
  
  // Durumu temizle
  st[motorIndex].aktif = false;
  st[motorIndex].bittiEdge = false;
  st[motorIndex].faz = FAZ_KAPALI;
  st[motorIndex].kalanPulse = 0;
}

void moveToHepsiniDurdur() {
  for (uint8_t m = 0; m < 3; m++) {
    moveToDurdur(m);
  }
}