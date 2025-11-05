// MoveTo.cpp - İVMELİ HAREKET MODÜLÜ v6 - SALINİM MODU EKLENDİ
// ═══════════════════════════════════════════════════════════════
// YENİ ÖZELLİK: Salınım modu parametresi eklendi
// - Normal mod: ACCEL_RAMP_X (6000p rampa)
// - Salınım modu: ACCEL_RAMP_X_SALINIM (2000p rampa - 3x hızlı!)
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
  
  // HEDEF BILGILERI (SABİT)
  long hedefEnc = 0;
  long baslangicEnc = 0;
  unsigned long toplamPulse = 0;
  
  // HAREKET BILGILERI
  MoveFaz faz = FAZ_KAPALI;
  unsigned int maxHz = 0;
  int yon = 0;
  
  // ✅ YENİ: SALINİM MODU BAYRAĞI
  bool salinimModu = false;
  
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
// MOTOR PARAMETRELERİ (✅ SALINİM DESTEĞİ İLE)
// ═══════════════════════════════════════════════════════════════
static unsigned long getRampaPulse(uint8_t motorIndex, MoveToState* s) {
  // ✅ SALINİM MODU KONTROLÜ (sadece X motor için)
  if (motorIndex == MOTOR_X && s->salinimModu) {
    return ACCEL_RAMP_X_SALINIM;  // 2000 pulse (hızlı rampa!)
  }
  
  // Normal modlar
  switch (motorIndex) {
    case MOTOR_Z:  return ACCEL_RAMP_Z;       // 6000
    case MOTOR_X:  return ACCEL_RAMP_X;       // 6000
    case MOTOR_B:  return ACCEL_RAMP_BIG;     // 400
    default:       return 400;
  }
}

static unsigned int getMinSpeed(uint8_t motorIndex, MoveToState* s) {
  // ✅ SALINİM MODU KONTROLÜ (sadece X motor için)
  if (motorIndex == MOTOR_X && s->salinimModu) {
    return MIN_SPEED_X_SALINIM;  // 200 Hz (hızlı başlangıç!)
  }
  
  // Normal modlar
  switch (motorIndex) {
    case MOTOR_Z:  return MIN_SPEED_Z;        // 100
    case MOTOR_X:  return MIN_SPEED_X;        // 100
    case MOTOR_B:  return MIN_SPEED_BIG;      // 100
    default:       return 50;
  }
}

// ═══════════════════════════════════════════════════════════════
// HIZ HESAPLAMA
// ═══════════════════════════════════════════════════════════════
static unsigned int hesaplaHiz(MoveToState* s, uint8_t motorIndex) {
  // ✅ State parametresini ilet
  unsigned long rampaPulse = getRampaPulse(motorIndex, s);
  unsigned int minSpeed = getMinSpeed(motorIndex, s);
  
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
// MOVETO - İVMELİ HAREKET BAŞLAT (✅ SALINİM MODU EKLENDİ)
// ═══════════════════════════════════════════════════════════════
bool moveTo(uint8_t motorIndex, long hedefEnc, unsigned int maxHz, bool salinimModu) {
  if (motorIndex > 2) {
    return false;
  }
  
  if (encoders[motorIndex] == nullptr) {
    return false;
  }
  
  if (st[motorIndex].aktif) {
    return false;
  }
  
  // ✅ Min speed kontrolü (salınım moduna göre)
  MoveToState tempState;
  tempState.salinimModu = salinimModu;
  unsigned int minSpeed = getMinSpeed(motorIndex, &tempState);
  
  if (maxHz < minSpeed) {
    return false;
  }
  
  long mevcutEnc = getEncoderPos(motorIndex);
  long fark = hedefEnc - mevcutEnc;
  
  // ✅ ZATEN HEDEFTEYSE (±2 tolerans)
  if (abs(fark) <= 2) {
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
  
  // ✅ SALINİM MODUNU KAYDET
  st[motorIndex].salinimModu = salinimModu;
  
  // ─────────────────────────────────────────────────────────────
  // FAZ BELİRLEME (✅ salınım moduna göre rampa)
  // ─────────────────────────────────────────────────────────────
  unsigned long rampaPulse = getRampaPulse(motorIndex, &st[motorIndex]);
  
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
      // ATILAN PULSE'U GÜNCELLE
      // ─────────────────────────────────────────────────────────
      st[m].atilanPulse += st[m].sonSegmentPulse;
      
      // ═══════════════════════════════════════════════════════════
      // TÜM PULSELER ATILDI MI?
      // ═══════════════════════════════════════════════════════════
      if (st[m].atilanPulse >= st[m].toplamPulse) {
        // ✅ BİTTİ - ENCODER KONTROLÜ YOK, SADECE RAPOR
        
        st[m].aktif = false;
        st[m].bittiEdge = true;
        st[m].faz = FAZ_KAPALI;
        st[m].salinimModu = false;  // ✅ Sıfırla
        
        pulseAtDurdur(m);
        
        continue;
      }
      
      // ─────────────────────────────────────────────────────────
      // HENÜZ BİTMEDİ - SONRAKİ SEGMENTİ BAŞLAT
      // ─────────────────────────────────────────────────────────
      unsigned long kalan = st[m].toplamPulse - st[m].atilanPulse;
      
      // FAZ GEÇİŞLERİ (✅ salınım moduna göre rampa)
      unsigned long rampaPulse = getRampaPulse(m, &st[m]);
      
      if (st[m].faz == FAZ_HIZLANMA && st[m].atilanPulse >= rampaPulse) {
        if (kalan <= rampaPulse) {
          st[m].faz = FAZ_YAVASLAMA;
        } else {
          st[m].faz = FAZ_SABIT_HIZ;
        }
      }
      else if (st[m].faz == FAZ_SABIT_HIZ && kalan <= rampaPulse) {
        st[m].faz = FAZ_YAVASLAMA;
      }
      
      // Sonraki segment hızı
      unsigned int hiz = hesaplaHiz(&st[m], m);
      unsigned long segmentPulse = hesaplaSegmentPulse(hiz);
      
      if (segmentPulse > kalan) {
        segmentPulse = kalan;
      }
      
      st[m].sonSegmentPulse = segmentPulse;
      
      useMotor(m);
      pulseAt(segmentPulse, st[m].yon, hiz);
    }
  }
}

// ═══════════════════════════════════════════════════════════════
// MOVETO BİTTİ Mİ? (EDGE DETECTION)
// ═══════════════════════════════════════════════════════════════
bool moveToBittiMi(uint8_t motorIndex) {
  if (motorIndex > 2) return false;
  
  bool bitti = st[motorIndex].bittiEdge;
  st[motorIndex].bittiEdge = false;
  return bitti;
}

// ═══════════════════════════════════════════════════════════════
// MOVETO DURDUR
// ═══════════════════════════════════════════════════════════════
void moveToDurdur(uint8_t motorIndex) {
  if (motorIndex > 2) return;
  
  pulseAtDurdur(motorIndex);
  
  noInterrupts();
  st[motorIndex].aktif = false;
  st[motorIndex].bittiEdge = false;
  st[motorIndex].faz = FAZ_KAPALI;
  st[motorIndex].salinimModu = false;  // ✅ Sıfırla
  interrupts();
}

// ═══════════════════════════════════════════════════════════════
// TÜMÜNÜ DURDUR
// ═══════════════════════════════════════════════════════════════
void moveToHepsiniDurdur() {
  for (uint8_t i = 0; i < 3; i++) {
    moveToDurdur(i);
  }
}

// ═══════════════════════════════════════════════════════════════
// MOVETO AKTİF Mİ?
// ═══════════════════════════════════════════════════════════════
bool moveToAktifMi(uint8_t motorIndex) {
  if (motorIndex > 2) return false;
  return st[motorIndex].aktif;
}

// ═══════════════════════════════════════════════════════════════
// DEĞİŞİKLİK ÖZET İ
// ═══════════════════════════════════════════════════════════════
/*

v6 YENİLİKLERİ (Salınım Modu):
──────────────────────────────────────────────────────────────

1. MoveToState struct'ına `salinimModu` bayrağı eklendi

2. getRampaPulse(motorIndex, state):
   - Normal: ACCEL_RAMP_X (6000p)
   - Salınım: ACCEL_RAMP_X_SALINIM (2000p) - 3x hızlı!

3. getMinSpeed(motorIndex, state):
   - Normal: MIN_SPEED_X (100 Hz)
   - Salınım: MIN_SPEED_X_SALINIM (200 Hz)

4. moveTo fonksiyon imzası:
   bool moveTo(motorIndex, hedefEnc, maxHz, salinimModu = false)
   
5. Geri uyumluluk:
   - Eski çağrılar: moveTo(MOTOR_X, 10000, 5000)
     → salinimModu = false (default)
   - Yeni çağrılar: moveTo(MOTOR_X, 10000, 8000, true)
     → salinimModu = true (hızlı rampa)

KULLANIM:
──────────────────────────────────────────────────────────────
// Normal mod (CiftOynatma geçişleri):
moveTo(MOTOR_X, 20000, 5000);        // 6000p rampa

// Salınım modu (MoveSalinim):
moveTo(MOTOR_X, 12000, 8000, true);  // 2000p rampa (HIZLI!)

*/