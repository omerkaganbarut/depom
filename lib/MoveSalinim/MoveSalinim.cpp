// MoveSalinim.cpp - X EKSENİ SALINİM MODÜLÜ v2.0
#include "MoveSalinim.h"
#include "MoveTo.h"
#include "Config.h"

// ═══════════════════════════════════════════════════════════════
// SALINİM DURUMU
// ═══════════════════════════════════════════════════════════════
struct MSState {
  bool aktif = false;
  
  long merkez = 0;           // Salınım merkezi (X1 veya X2)
  long offset = 0;           // Salınım genliği (±offset)
  unsigned int hiz = 0;      // X motor hızı (Hz)
  
  bool sagaTaraf = true;     // true = merkez+offset, false = merkez-offset
  bool hareketBasladi = false;
};

static MSState MS;

// ═══════════════════════════════════════════════════════════════
// ENCODER POINTER
// ═══════════════════════════════════════════════════════════════
static StepMotorEncoder* gXEnc = nullptr;

// ═══════════════════════════════════════════════════════════════
// ENCODER SETUP
// ═══════════════════════════════════════════════════════════════
void msEncoderSetup(StepMotorEncoder* xEncoder) {
  gXEnc = xEncoder;
}

// ═══════════════════════════════════════════════════════════════
// SALINİM BAŞLAT
// ═══════════════════════════════════════════════════════════════
bool msBaslat(long offset, unsigned int hiz) {
  // ───────────────────────────────────────────────────────────
  // PARAMETRE KONTROLÜ
  // ───────────────────────────────────────────────────────────
  if (!gXEnc) {
    Serial.println("MS: Encoder setup yapilmamis!");
    return false;
  }
  
  if (offset == 0) {
    Serial.println("MS: Offset 0 olamaz!");
    return false;
  }
  
  if (hiz == 0) {
    Serial.println("MS: Hiz 0 olamaz!");
    return false;
  }
  
  // ───────────────────────────────────────────────────────────
  // MERKEZ POZİSYONU BELİRLE (şu anki X konumu)
  // ───────────────────────────────────────────────────────────
  MS.merkez = gXEnc->getPosition();
  MS.offset = abs(offset);  // Negatif olsa bile pozitif yap
  MS.hiz = hiz;
  MS.sagaTaraf = true;      // İlk hareket sağa (merkez + offset)
  MS.hareketBasladi = false;
  MS.aktif = true;
  
 
  
  return true;
}

// ═══════════════════════════════════════════════════════════════
// SALINİM ARKA PLAN DÖNGÜSÜ
// ═══════════════════════════════════════════════════════════════
void msRun() {
  if (!MS.aktif) return;
  
  // ───────────────────────────────────────────────────────────
  // İLK HAREKETI BAŞLAT
  // ───────────────────────────────────────────────────────────
  if (!MS.hareketBasladi) {
    long hedef;
    
    if (MS.sagaTaraf) {
      hedef = MS.merkez + MS.offset;  // Sağa git
    } else {
      hedef = MS.merkez - MS.offset;  // Sola git
    }
    
   
    // ✅ SALINİM MODU AKTİF (hızlı rampa - 2000p)
    moveTo(MOTOR_X, hedef, MS.hiz, true);
    MS.hareketBasladi = true;
    return;
  }
  
  // ───────────────────────────────────────────────────────────
  // HEDEFE ULAŞTI MI?
  // ───────────────────────────────────────────────────────────
  if (moveToBittiMi(MOTOR_X)) {
    // Yön değiştir (sağ ↔ sol)
    MS.sagaTaraf = !MS.sagaTaraf;
    MS.hareketBasladi = false;  // Yeni hareket başlat
  }
}

// ═══════════════════════════════════════════════════════════════
// AKTİF Mİ?
// ═══════════════════════════════════════════════════════════════
bool msAktifMi() {
  return MS.aktif;
}

// ═══════════════════════════════════════════════════════════════
// DURDUR
// ═══════════════════════════════════════════════════════════════
void msDurdur() {
  if (!MS.aktif) return;
  
  // MoveTo'yu durdur
  moveToDurdur(MOTOR_X);
  
  MS.aktif = false;
  MS.hareketBasladi = false;
  
  Serial.println("MS: Salinim durduruldu!");
}

// ═══════════════════════════════════════════════════════════════
// DURUM BİLGİSİ
// ═══════════════════════════════════════════════════════════════
bool msDurumBilgisi(long* merkez, long* offset, long* hedef) {
  if (!MS.aktif) return false;
  
  if (merkez) *merkez = MS.merkez;
  if (offset) *offset = MS.offset;
  if (hedef) {
    *hedef = MS.sagaTaraf ? (MS.merkez + MS.offset) : (MS.merkez - MS.offset);
  }
  
  return true;
}