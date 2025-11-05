// CiftKayitModulu.cpp - v6.0 CLEAN (Gereksiz kontroller kaldırıldı)
// ═══════════════════════════════════════════════════════════════
// DEĞİŞİKLİKLER:
// - X zaten hedefteyse kontrolü KALDIRILDI
// - Daha basit geçiş mantığı
// ═══════════════════════════════════════════════════════════════

#include "CiftKayitModulu.h"
#include "Config.h"
#include "KayitModulu.h"
#include "MoveTo.h"
#include "CiftOynatmaModulu.h"

// ═══════════════════════════════════════════════════════════════
// DURUM MAKİNESİ
// ═══════════════════════════════════════════════════════════════
enum CKDurum {
  CK_KAPALI = 0,
  CK_X1_GIDIYOR,
  CK_X1_ONAY_BEKLE,
  CK_KAYIT1_CALISIYOR,
  CK_X2_GIDIYOR,
  CK_X2_ONAY_BEKLE,
  CK_KAYIT2_CALISIYOR,
  CK_TAMAMLANDI
};

// ═══════════════════════════════════════════════════════════════
// STATİK DEĞİŞKENLER
// ═══════════════════════════════════════════════════════════════
static CKDurum durum = CK_KAPALI;

static StepMotorEncoder* bigEnc = nullptr;
static StepMotorEncoder* xEnc = nullptr;

static long x1Hedef = 0;
static long x2Hedef = 0;
static int yon1 = 0;
static int yon2 = 1;

// GLOBAL KAYIT ARRAYLERI
CK_Sample kayit1[KAYIT_ORNEK_SAYISI];
CK_Sample kayit2[KAYIT_ORNEK_SAYISI];

// Global A0 aralığı
uint16_t globalA0Min = 1023;
uint16_t globalA0Max = 0;

// ═══════════════════════════════════════════════════════════════
// ENCODER SETUP
// ═══════════════════════════════════════════════════════════════
void ckEncoderSetup(StepMotorEncoder* bigEncoder, StepMotorEncoder* xEncoder) {
  bigEnc = bigEncoder;
  xEnc = xEncoder;
}

// ═══════════════════════════════════════════════════════════════
// ÇİFT KAYIT BAŞLATMA
// ═══════════════════════════════════════════════════════════════
void ckBaslat(long x1Enc, long x2Enc, int kayit1Yon, int kayit2Yon) {
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║           ÇİFT KAYIT BAŞLATILIYOR              ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
  
  coZSifirlamaReset();
  
  x1Hedef = x1Enc;
  x2Hedef = x2Enc;
  yon1 = kayit1Yon;
  yon2 = kayit2Yon;
  
  Serial.print(F("  X1 Pozisyonu: "));
  Serial.print(x1Hedef);
  Serial.print(F(" (Yön: "));
  Serial.print(yon1 ? F("Geri") : F("İleri"));
  Serial.println(F(")"));
  
  Serial.print(F("  X2 Pozisyonu: "));
  Serial.print(x2Hedef);
  Serial.print(F(" (Yön: "));
  Serial.print(yon2 ? F("Geri") : F("İleri"));
  Serial.println(F(")"));
  
  if (bigEnc == nullptr || xEnc == nullptr) {
    Serial.println(F("\n✗ Hata: Encoder'lar ayarlanmamış!"));
    return;
  }
  
  // ─────────────────────────────────────────────────────────────
  // X1'E GİT
  // ─────────────────────────────────────────────────────────────
  Serial.println(F("\n[ADIM 1/6] X1 pozisyonuna gidiliyor..."));
  
  moveTo(MOTOR_X, x1Hedef, 10000,false);
  durum = CK_X1_GIDIYOR;
}

// ═══════════════════════════════════════════════════════════════
// ÇİFT KAYIT ARKA PLAN
// ═══════════════════════════════════════════════════════════════
void ckRun() {
  switch (durum) {
    
    case CK_KAPALI:
      return;
    
    // ───────────────────────────────────────────────────────────
    case CK_X1_GIDIYOR:
    // ───────────────────────────────────────────────────────────
      if (!moveToAktifMi(MOTOR_X)) {
        Serial.println(F("✓ X1 pozisyonuna ulaşıldı!\n"));
        Serial.print(F("  Mevcut X: "));
        Serial.println(xEnc->getPosition());
        
        Serial.println(F("\n[ADIM 2/6] Kayıt1 başlatmaya hazır."));
        Serial.println(F("───────────────────────────────────────────────"));
        Serial.println(F("  Kayıt1'i başlatmak için 'Y' tuşuna basın."));
        Serial.println(F("  İptal için 'N' tuşuna basın."));
        Serial.print(F("  > "));
        
        durum = CK_X1_ONAY_BEKLE;
      }
      break;
    
    // ───────────────────────────────────────────────────────────
    case CK_X1_ONAY_BEKLE:
    // ───────────────────────────────────────────────────────────
      if (Serial.available() > 0) {
        char c = Serial.peek();
        
        if (c == 'Y' || c == 'y') {
          Serial.read();
          Serial.println(F("Y\n"));
          
          Serial.println(F("[ADIM 3/6] Kayıt1 başlatılıyor...\n"));
          kayitBaslat(yon1);
          
          durum = CK_KAYIT1_CALISIYOR;
        }
        else if (c == 'N' || c == 'n') {
          Serial.read();
          Serial.println(F("N\n"));
          Serial.println(F("✗ Çift kayıt iptal edildi!"));
          
          durum = CK_KAPALI;
        }
      }
      break;
    
    // ───────────────────────────────────────────────────────────
    case CK_KAYIT1_CALISIYOR:
    // ───────────────────────────────────────────────────────────
      if (kayitTamamlandiMi()) {
        Serial.println(F("\n✓ Kayıt1 tamamlandı!\n"));
        
        // KAYIT1'İ KOPYALA
        const KM_Sample* src = kayitVerileri();
        for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
          kayit1[i].enc = src[i].enc;
          kayit1[i].a0 = src[i].a0;
        }
        
        Serial.println(F("→ Kayıt1 kaydedildi."));
        Serial.print(F("  Örnek sayısı: "));
        Serial.println(kayitOrnekSayisi());
        
        Serial.println(F("\n[ADIM 4/6] X2 pozisyonuna gidiliyor..."));
        
        moveTo(MOTOR_X, x2Hedef, 10000,false);
        durum = CK_X2_GIDIYOR;
      }
      break;
    
    // ───────────────────────────────────────────────────────────
    case CK_X2_GIDIYOR:
    // ───────────────────────────────────────────────────────────
      if (!moveToAktifMi(MOTOR_X)) {
        Serial.println(F("✓ X2 pozisyonuna ulaşıldı!\n"));
        Serial.print(F("  Mevcut X: "));
        Serial.println(xEnc->getPosition());
        
        Serial.println(F("\n[ADIM 5/6] Kayıt2 başlatmaya hazır."));
        Serial.println(F("───────────────────────────────────────────────"));
        Serial.println(F("  Kayıt2'yi başlatmak için 'Y' tuşuna basın."));
        Serial.println(F("  İptal için 'N' tuşuna basın."));
        Serial.print(F("  > "));
        
        durum = CK_X2_ONAY_BEKLE;
      }
      break;
    
    // ───────────────────────────────────────────────────────────
    case CK_X2_ONAY_BEKLE:
    // ───────────────────────────────────────────────────────────
      if (Serial.available() > 0) {
        char c = Serial.peek();
        
        if (c == 'Y' || c == 'y') {
          Serial.read();
          Serial.println(F("Y\n"));
          
          Serial.println(F("[ADIM 6/6] Kayıt2 başlatılıyor...\n"));
          kayitBaslat(yon2);
          
          durum = CK_KAYIT2_CALISIYOR;
        }
        else if (c == 'N' || c == 'n') {
          Serial.read();
          Serial.println(F("N\n"));
          Serial.println(F("✗ Kayıt2 iptal edildi!"));
          
          durum = CK_KAPALI;
        }
      }
      break;
    
    // ───────────────────────────────────────────────────────────
    case CK_KAYIT2_CALISIYOR:
    // ───────────────────────────────────────────────────────────
      if (kayitTamamlandiMi()) {
        Serial.println(F("\n✓ Kayıt2 tamamlandı!\n"));
        
        // KAYIT2'Yİ KOPYALA
        const KM_Sample* src = kayitVerileri();
        for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
          kayit2[i].enc = src[i].enc;
          kayit2[i].a0 = src[i].a0;
        }
        
        // GLOBAL A0 MIN/MAX HESAPLA
        globalA0Min = 1023;
        globalA0Max = 0;
        
        for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
          if (kayit1[i].a0 < globalA0Min) globalA0Min = kayit1[i].a0;
          if (kayit1[i].a0 > globalA0Max) globalA0Max = kayit1[i].a0;
        }
        
        for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
          if (kayit2[i].a0 < globalA0Min) globalA0Min = kayit2[i].a0;
          if (kayit2[i].a0 > globalA0Max) globalA0Max = kayit2[i].a0;
        }
        
        Serial.println(F("\n╔════════════════════════════════════════════════╗"));
        Serial.println(F("║          ÇİFT KAYIT TAMAMLANDI! ✓              ║"));
        Serial.println(F("╚════════════════════════════════════════════════╝\n"));
        

        // ═══════════════════════════════════════════════════════════
        // 📊 KAYIT1 VERİLERİ
        // ═══════════════════════════════════════════════════════════
        Serial.println(F("─────────────────────────────────────────────────"));
        Serial.println(F("📋 KAYIT1 VERİLERİ:"));
        Serial.println(F("─────────────────────────────────────────────────"));
        Serial.println(F("  # | BIG Enc |  A0  |"));
        Serial.println(F("────┼─────────┼──────┤"));
        
        for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
          Serial.print(F("  "));
          if (i < 10) Serial.print(F(" "));
          Serial.print(i);
          Serial.print(F(" | "));
          
          if (kayit1[i].enc < 10000) Serial.print(F(" "));
          if (kayit1[i].enc < 1000) Serial.print(F(" "));
          if (kayit1[i].enc < 100) Serial.print(F(" "));
          if (kayit1[i].enc < 10) Serial.print(F(" "));
          Serial.print(kayit1[i].enc);
          Serial.print(F(" | "));
          
          if (kayit1[i].a0 < 1000) Serial.print(F(" "));
          if (kayit1[i].a0 < 100) Serial.print(F(" "));
          if (kayit1[i].a0 < 10) Serial.print(F(" "));
          Serial.print(kayit1[i].a0);
          Serial.println(F(" |"));
        }
        
        // ═══════════════════════════════════════════════════════════
        // 📊 KAYIT2 VERİLERİ
        // ═══════════════════════════════════════════════════════════
        Serial.println(F("\n─────────────────────────────────────────────────"));
        Serial.println(F("📋 KAYIT2 VERİLERİ:"));
        Serial.println(F("─────────────────────────────────────────────────"));
        Serial.println(F("  # | BIG Enc |  A0  |"));
        Serial.println(F("────┼─────────┼──────┤"));
        
        for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
          Serial.print(F("  "));
          if (i < 10) Serial.print(F(" "));
          Serial.print(i);
          Serial.print(F(" | "));
          
          if (kayit2[i].enc < 10000) Serial.print(F(" "));
          if (kayit2[i].enc < 1000) Serial.print(F(" "));
          if (kayit2[i].enc < 100) Serial.print(F(" "));
          if (kayit2[i].enc < 10) Serial.print(F(" "));
          Serial.print(kayit2[i].enc);
          Serial.print(F(" | "));
          
          if (kayit2[i].a0 < 1000) Serial.print(F(" "));
          if (kayit2[i].a0 < 100) Serial.print(F(" "));
          if (kayit2[i].a0 < 10) Serial.print(F(" "));
          Serial.print(kayit2[i].a0);
          Serial.println(F(" |"));
        }

        Serial.println(F("─────────────────────────────────────────────────"));
        Serial.println(F("GLOBAL A0 ARALIĞI:"));
        Serial.print(F("  Min   : "));
        Serial.println(globalA0Min);
        Serial.print(F("  Max   : "));
        Serial.println(globalA0Max);
        Serial.print(F("  Aralık: "));
        Serial.println(globalA0Max - globalA0Min);
        Serial.println(F("─────────────────────────────────────────────────\n"));
        
        Serial.println(F("✓ Çift oynatma için hazır!"));
        Serial.println(F("  Komut: CO (Çift Oynatma)\n"));
        
        durum = CK_TAMAMLANDI;
      }
      break;
    
    case CK_TAMAMLANDI:
      return;
  }
}

// ═══════════════════════════════════════════════════════════════
// DURUM FONKSİYONLARI
// ═══════════════════════════════════════════════════════════════

bool ckAktifMi() {
  return (durum != CK_KAPALI && durum != CK_TAMAMLANDI);
}

bool ckTamamlandiMi() {
  return (durum == CK_TAMAMLANDI);
}

// ═══════════════════════════════════════════════════════════════
// LİSTELEME FONKSİYONLARI
// ═══════════════════════════════════════════════════════════════

void ckKayit1Listele() {
  if (durum != CK_TAMAMLANDI) {
    Serial.println(F("✗ Kayıt1 henüz tamamlanmadı!"));
    return;
  }
  
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║                  KAYIT 1                       ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
  
  Serial.println(F(" IDX |  ENCODER  |   A0  |  ENC FARK"));
  Serial.println(F("─────┼───────────┼───────┼──────────"));
  
  for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
    long encFark = 0;
    if (i > 0) {
      encFark = kayit1[i].enc - kayit1[i-1].enc;
    }
    
    char buf[48];
    sprintf(buf, " %3d | %9ld | %5d | %+9ld", 
            i, kayit1[i].enc, kayit1[i].a0, encFark);
    Serial.println(buf);
  }
  Serial.println();
}

void ckKayit2Listele() {
  if (durum != CK_TAMAMLANDI) {
    Serial.println(F("✗ Kayıt2 henüz tamamlanmadı!"));
    return;
  }
  
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║                  KAYIT 2                       ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
  
  Serial.println(F(" IDX |  ENCODER  |   A0  |  ENC FARK"));
  Serial.println(F("─────┼───────────┼───────┼──────────"));
  
  for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
    long encFark = 0;
    if (i > 0) {
      encFark = kayit2[i].enc - kayit2[i-1].enc;
    }
    
    char buf[48];
    sprintf(buf, " %3d | %9ld | %5d | %+9ld", 
            i, kayit2[i].enc, kayit2[i].a0, encFark);
    Serial.println(buf);
  }
  Serial.println();
}

// ═══════════════════════════════════════════════════════════════
// DURDURMA
// ═══════════════════════════════════════════════════════════════
void ckDurdur() {
  Serial.println(F("\n[ÇİFT KAYIT] Durduruldu!"));
  
  moveToDurdur(MOTOR_X);
  moveToDurdur(MOTOR_B);
  
  kayitDurdur();
  
  durum = CK_KAPALI;
  
  Serial.println(F("ℹ️  Not: Mevcut kayıtlar saklandı.\n"));
}