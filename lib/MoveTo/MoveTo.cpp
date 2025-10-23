// MoveTo.cpp - İVMELİ HAREKET MODÜLÜ v3 - MOTOR BAZLI RAMPA + KESİN HEDEF
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
  
  // HEDEF BILGILERI (SABİT - HİÇ DEĞİŞMEZ!)
  long hedefEnc = 0;
  long baslangicEnc = 0;
  unsigned long toplamPulse = 0;  // TOPLAM atılacak pulse (SABİT!)
  
  // HAREKET BILGILERI
  MoveFaz faz = FAZ_KAPALI;
  unsigned int maxHz = 0;
  int yon = 0;
  
  // SEGMENT TAKIBI
  unsigned long atilanPulse = 0;        // Şu ana kadar atılan pulse
  unsigned long sonSegmentPulse = 0;    // Son segment'in pulse sayısı
  
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
    case MOTOR_Z:  return ACCEL_RAMP_Z;   // 4000
    case MOTOR_X:  return ACCEL_RAMP_X;   // 3000
    case MOTOR_B:  return ACCEL_RAMP_BIG; // 200
    default:       return 200;
  }
}

static unsigned int getMinSpeed(uint8_t motorIndex) {
  switch (motorIndex) {
    case MOTOR_Z:  return MIN_SPEED_Z;    // 20 Hz
    case MOTOR_X:  return MIN_SPEED_X;    // 30 Hz
    case MOTOR_B:  return MIN_SPEED_BIG;  // 50 Hz
    default:       return 50;
  }
}

// ═══════════════════════════════════════════════════════════════
// HIZ HESAPLAMA (Motor bazlı)
// ═══════════════════════════════════════════════════════════════
static unsigned int hesaplaHiz(MoveToState* s, uint8_t motorIndex) {
  unsigned long rampaPulse = getRampaPulse(motorIndex);
  unsigned int minSpeed = getMinSpeed(motorIndex);
  
  if (s->faz == FAZ_HIZLANMA) {
    // İlk rampaPulse kadar pulse'da hızlan
    double oran = (double)s->atilanPulse / (double)rampaPulse;
    if (oran > 1.0) oran = 1.0;
    
    unsigned int hiz = minSpeed + (unsigned int)((s->maxHz - minSpeed) * oran);
    return hiz;
  }
  else if (s->faz == FAZ_SABIT_HIZ) {
    return s->maxHz;
  }
  else if (s->faz == FAZ_YAVASLAMA) {
    // Son rampaPulse kadar pulse'da yavaşla
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
  // YENİ: DURUM BAŞLATMA (Kesin pulse hesabı)
  // ─────────────────────────────────────────────────────────────
  st[motorIndex].aktif = true;
  st[motorIndex].bittiEdge = false;
  st[motorIndex].hedefEnc = hedefEnc;
  st[motorIndex].baslangicEnc = mevcutEnc;
  st[motorIndex].maxHz = maxHz;
  st[motorIndex].toplamPulse = pulse;     // ← SABİT KALIR!
  st[motorIndex].atilanPulse = 0;         // ← Sıfırdan başla
  st[motorIndex].yon = yon;
  st[motorIndex].baslangicMs = millis();
  
  // ─────────────────────────────────────────────────────────────
  // FAZ BELİRLEME (Motor bazlı rampa)
  // ─────────────────────────────────────────────────────────────
  unsigned long rampaPulse = getRampaPulse(motorIndex);
  
  if (pulse <= rampaPulse) {
    st[motorIndex].faz = FAZ_YAVASLAMA;  // Sadece yavaşlama
  } 
  else if (pulse <= 2 * rampaPulse) {
    st[motorIndex].faz = FAZ_HIZLANMA;   // Hızlanma + yavaşlama
  } 
  else {
    st[motorIndex].faz = FAZ_HIZLANMA;   // Tam profil
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
      // ✅ ATILAN PULSE'U GÜNCELLE (Kesin sayım)
      // ─────────────────────────────────────────────────────────
      st[m].atilanPulse += st[m].sonSegmentPulse;
      
      // ─────────────────────────────────────────────────────────
      // ✅ TÜM PULSE'LAR ATILDI MI? (Matematik olarak kesin!)
      // ─────────────────────────────────────────────────────────
      if (st[m].atilanPulse >= st[m].toplamPulse) {
        
        // ✅ İŞ BİTTİ - KESIN HEDEFE ULAŞILDI!
        moveToDurdur(m);
        st[m].bittiEdge = true;
        
        // ─────────────────────────────────────────────────────
        // ✅ ENCODER KONTROLÜ (Bilgi amaçlı)
        // ─────────────────────────────────────────────────────
        long finalEnc = getEncoderPos(m);
        long fark = st[m].hedefEnc - finalEnc;
        
        if (abs(fark) > 10) {
          Serial.print(F("[MoveTo] Motor "));
          Serial.print(m);
          Serial.print(F(" sapma: "));
          Serial.print(fark);
          Serial.println(F("p (Encoder kayması)"));
        }
        
        continue;
      }
      
      // ─────────────────────────────────────────────────────────
      // FAZ GEÇİŞLERİ (atılan pulse'a göre)
      // ─────────────────────────────────────────────────────────
      unsigned long rampaPulse = getRampaPulse(m);
      
      if (st[m].faz == FAZ_HIZLANMA) {
        if (st[m].atilanPulse >= rampaPulse) {
          unsigned long kalan = st[m].toplamPulse - st[m].atilanPulse;
          
          if (kalan > rampaPulse) {
            st[m].faz = FAZ_SABIT_HIZ;
          } else {
            st[m].faz = FAZ_YAVASLAMA;
          }
        }
      }
      else if (st[m].faz == FAZ_SABIT_HIZ) {
        unsigned long kalan = st[m].toplamPulse - st[m].atilanPulse;
        
        if (kalan <= rampaPulse) {
          st[m].faz = FAZ_YAVASLAMA;
        }
      }
      
      // ─────────────────────────────────────────────────────────
      // SONRAKİ SEGMENTİ BAŞLAT
      // ─────────────────────────────────────────────────────────
      unsigned int hiz = hesaplaHiz(&st[m], m);
      unsigned long kalan = st[m].toplamPulse - st[m].atilanPulse;
      unsigned long segmentPulse = hesaplaSegmentPulse(hiz);
      
      if (segmentPulse > kalan) {
        segmentPulse = kalan;  // ← Son segment kırpılır
      }
      
      st[m].sonSegmentPulse = segmentPulse;
      
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
  return (long)(st[motorIndex].toplamPulse - st[motorIndex].atilanPulse);
}

// ═══════════════════════════════════════════════════════════════
// DURDURMA FONKSİYONLARI
// ═══════════════════════════════════════════════════════════════
void moveToDurdur(uint8_t motorIndex) {
  if (motorIndex > 2) {
    return;
  }
  
  // ✅ ÖNCE DURUMU TEMİZLE
  st[motorIndex].aktif = false;
  st[motorIndex].bittiEdge = false;
  st[motorIndex].faz = FAZ_KAPALI;
  st[motorIndex].atilanPulse = 0;
  st[motorIndex].toplamPulse = 0;
  
  // ✅ SONRA PULSEAT'I DURDUR (içinde ENA HIGH yapıyor)
  pulseAtDurdur(motorIndex);
}

void moveToHepsiniDurdur() {
  for (uint8_t m = 0; m < 3; m++) {
    moveToDurdur(m);
  }
}