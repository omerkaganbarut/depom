// CiftKayitModulu.cpp - İKİ FARKLI X POZİSYONUNDA KAYIT ALMA
// ═══════════════════════════════════════════════════════════════
// AMAÇ: X1 ve X2 noktalarında kayıt alıp Global A0_min/max hesapla
// ═══════════════════════════════════════════════════════════════

#include "CiftKayitModulu.h"
#include "Config.h"
#include "KayitModulu.h"
#include "MoveTo.h"
#include "CiftOynatmaModulu.h"  // ✅ coZSifirlamaReset() için

// ═══════════════════════════════════════════════════════════════
// DURUM MAKİNESİ
// ═══════════════════════════════════════════════════════════════
// AÇIKLAMA: Her durum bir adımı temsil eder. Non-blocking yapı.
enum CKDurum {
  CK_KAPALI = 0,              // İşlem başlatılmamış
  
  // ─────────────────────────────────────────────────────────────
  // KAYIT1 AŞAMASI (X1 noktası)
  // ─────────────────────────────────────────────────────────────
  CK_X1_GIDIYOR,              // MoveTo ile X1'e gidiyor
  CK_X1_ONAY_BEKLE,           // "Kayıt1 başlasın mı?" (Y/N)
  CK_KAYIT1_CALISIYOR,        // kayitBaslat() çağrıldı, kayitRun() aktif
  CK_X2_ONAY_BEKLE,           // "X2'ye git?" (Y/N)
  
  // ─────────────────────────────────────────────────────────────
  // KAYIT2 AŞAMASI (X2 noktası)
  // ─────────────────────────────────────────────────────────────
  CK_X2_GIDIYOR,              // MoveTo ile X2'ye gidiyor
  CK_X2_ONAY2_BEKLE,          // "Kayıt2 başlasın mı?" (Y/N)
  CK_KAYIT2_CALISIYOR,        // kayitBaslat() çağrıldı, kayitRun() aktif
  
  CK_TAMAMLANDI               // Her iki kayıt da alındı
};

// ═══════════════════════════════════════════════════════════════
// STATİK DEĞİŞKENLER
// ═══════════════════════════════════════════════════════════════
static CKDurum durum = CK_KAPALI;

// Encoder pointer'ları
static StepMotorEncoder* bigEnc = nullptr;
static StepMotorEncoder* xEnc = nullptr;

// Kullanıcı parametreleri
static long x1Hedef = 0;          // X1 pozisyonu
static long x2Hedef = 0;          // X2 pozisyonu
static int yon1 = 0;              // Kayıt1 yönü (0=ileri)
static int yon2 = 0;              // Kayıt2 yönü (1=geri)

// Kayıt verileri (2 ayrı dizi - 101 örnek)
static CK_Sample kayit1[KAYIT_ORNEK_SAYISI];
static CK_Sample kayit2[KAYIT_ORNEK_SAYISI];

// Global A0 aralığı (2 kayıttan hesaplanır)
static uint16_t globalA0Min = 1023;
static uint16_t globalA0Max = 0;

// ═══════════════════════════════════════════════════════════════
// ENCODER SETUP (setup() içinde bir kez çağrılır)
// ═══════════════════════════════════════════════════════════════
void ckEncoderSetup(StepMotorEncoder* bigEncoder, StepMotorEncoder* xEncoder) {
  bigEnc = bigEncoder;
  xEnc = xEncoder;
}

// ═══════════════════════════════════════════════════════════════
// ÇİFT KAYIT BAŞLATMA
// ═══════════════════════════════════════════════════════════════
// PARAMETRELER:
//   x1Enc      : X1 pozisyonu (encoder değeri)
//   x2Enc      : X2 pozisyonu (encoder değeri)
//   kayit1Yon  : Kayıt1 yönü (0=ileri 0→16000, 1=geri 16000→0)
//   kayit2Yon  : Kayıt2 yönü
// ═══════════════════════════════════════════════════════════════
void ckBaslat(long x1Enc, long x2Enc, int kayit1Yon, int kayit2Yon) {
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║           ÇİFT KAYIT BAŞLATILIYOR              ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
  
  // ─────────────────────────────────────────────────────────────
  // Z SIFIRLAMA BAYRAĞINI SIFIRLA
  // ─────────────────────────────────────────────────────────────
  // AÇIKLAMA: Yeni kayıt alındığında bir sonraki CO komutunda
  //           Z sıfırlama aşaması tekrar çalışmalı.
  coZSifirlamaReset();
  
  // ─────────────────────────────────────────────────────────────
  // PARAMETRELERE KAYDET
  // ─────────────────────────────────────────────────────────────
  x1Hedef = x1Enc;
  x2Hedef = x2Enc;
  yon1 = kayit1Yon;
  yon2 = kayit2Yon;
  
  // ─────────────────────────────────────────────────────────────
  // RAPOR
  // ─────────────────────────────────────────────────────────────
  Serial.print(F("  X1 Pozisyonu : "));
  Serial.print(x1Hedef);
  Serial.print(F(" (Yön: "));
  Serial.print(yon1 ? F("Geri") : F("İleri"));
  Serial.println(F(")"));
  
  Serial.print(F("  X2 Pozisyonu : "));
  Serial.print(x2Hedef);
  Serial.print(F(" (Yön: "));
  Serial.print(yon2 ? F("Geri") : F("İleri"));
  Serial.println(F(")"));
  
  // ─────────────────────────────────────────────────────────────
  // ENCODER KONTROL
  // ─────────────────────────────────────────────────────────────
  if (bigEnc == nullptr || xEnc == nullptr) {
    Serial.println(F("\n✗ Hata: Encoder'lar ayarlanmamış!"));
    Serial.println(F("  setup() içinde ckEncoderSetup() çağırın.\n"));
    return;
  }
  
  // ─────────────────────────────────────────────────────────────
  // [ADIM 1/7] X1'E GİT
  // ─────────────────────────────────────────────────────────────
  Serial.println(F("\n[ADIM 1/7] X1 pozisyonuna gidiliyor..."));
  if (!moveTo(MOTOR_X, x1Hedef, 100)) {
    Serial.println(F("✗ X motor hareket başlatılamadı!"));
    return;
  }
  
  durum = CK_X1_GIDIYOR;
}

// ═══════════════════════════════════════════════════════════════
// ÇİFT KAYIT ARKA PLAN (Her loop'ta çağrılır)
// ═══════════════════════════════════════════════════════════════
// AÇIKLAMA: Durum makinesini çalıştırır. MoveTo ve KayitModulu'nun
//           arka planlarını çağırır. Y/N onaylarını bekler.
// ═══════════════════════════════════════════════════════════════
void ckRun() {
  switch (durum) {
    
    // ───────────────────────────────────────────────────────────
    case CK_KAPALI:
    // ───────────────────────────────────────────────────────────
      // Hiçbir şey yapma (bekleme modunda)
      return;
    
    // ───────────────────────────────────────────────────────────
    case CK_X1_GIDIYOR:
    // ───────────────────────────────────────────────────────────
      // AÇIKLAMA: MoveTo arka planı main loop'ta çağrılıyor (moveToRun).
      //           Burada sadece "bitti mi?" kontrolü yapıyoruz.
      
      if (moveToBittiMi(MOTOR_X)) {
        Serial.println(F("✓ X1 pozisyonuna ulaşıldı!\n"));
        
        Serial.print(F("  Mevcut X: "));
        Serial.println(xEnc->getPosition());
        
        Serial.println(F("\n[ADIM 2/7] Kayıt1 hazır."));
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
      // AÇIKLAMA: Kullanıcıdan Y/N girişi bekliyoruz.
      //           Y = Kayıt1 başlat, N = İptal et
      
      if (Serial.available()) {
        char c = Serial.read();
        
        if (c == 'Y' || c == 'y') {
          Serial.println(F("Y\n"));
          
          // [ADIM 3/7] KAYIT1 BAŞLAT
          Serial.println(F("[ADIM 3/7] Kayıt1 başlatılıyor...\n"));
          kayitBaslat(yon1);
          
          durum = CK_KAYIT1_CALISIYOR;
        }
        else if (c == 'N' || c == 'n') {
          Serial.println(F("N\n"));
          Serial.println(F("✗ Çift kayıt iptal edildi!\n"));
          durum = CK_KAPALI;
        }
      }
      break;
    
    // ───────────────────────────────────────────────────────────
    case CK_KAYIT1_CALISIYOR:
    // ───────────────────────────────────────────────────────────
      // AÇIKLAMA: KayitModulu arka planı main loop'ta çağrılıyor (kayitRun).
      //           Burada sadece "tamamlandı mı?" kontrolü yapıyoruz.
      
      if (kayitTamamlandiMi()) {
        Serial.println(F("\n✓ Kayıt1 tamamlandı!\n"));
        
        // ═════════════════════════════════════════════════════════
        // KAYIT1'İ KOPYALA (KayitModulu'nden bu modülün dizisine)
        // ═════════════════════════════════════════════════════════
        const KM_Sample* src = kayitVerileri();
        for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
          kayit1[i].enc = src[i].enc;
          kayit1[i].a0 = src[i].a0;
        }
        
        Serial.println(F("[ADIM 4/7] X2'ye geçiş hazır."));
        Serial.println(F("───────────────────────────────────────────────"));
        Serial.println(F("  X2 pozisyonuna gitmek için 'Y' tuşuna basın."));
        Serial.println(F("  İptal için 'N' tuşuna basın."));
        Serial.print(F("  > "));
        
        durum = CK_X2_ONAY_BEKLE;
      }
      break;
    
    // ───────────────────────────────────────────────────────────
    case CK_X2_ONAY_BEKLE:
    // ───────────────────────────────────────────────────────────
      // AÇIKLAMA: Kullanıcıdan Y/N girişi bekliyoruz.
      //           Y = X2'ye git, N = İptal et
      
      if (Serial.available()) {
        char c = Serial.read();
        
        if (c == 'Y' || c == 'y') {
          Serial.println(F("Y\n"));
          
          // [ADIM 5/7] X2'YE GİT
          Serial.println(F("[ADIM 5/7] X2 pozisyonuna gidiliyor..."));
          if (!moveTo(MOTOR_X, x2Hedef, 100)) {
            Serial.println(F("✗ X motor hareket başlatılamadı!"));
            durum = CK_KAPALI;
            return;
          }
          
          durum = CK_X2_GIDIYOR;
        }
        else if (c == 'N' || c == 'n') {
          Serial.println(F("N\n"));
          Serial.println(F("✗ Çift kayıt iptal edildi!\n"));
          durum = CK_KAPALI;
        }
      }
      break;
    
    // ───────────────────────────────────────────────────────────
    case CK_X2_GIDIYOR:
    // ───────────────────────────────────────────────────────────
      // AÇIKLAMA: MoveTo arka planı çalışıyor (moveToRun).
      
      if (moveToBittiMi(MOTOR_X)) {
        Serial.println(F("✓ X2 pozisyonuna ulaşıldı!\n"));
        
        Serial.print(F("  Mevcut X: "));
        Serial.println(xEnc->getPosition());
        
        Serial.println(F("\n[ADIM 6/7] Kayıt2 hazır."));
        Serial.println(F("───────────────────────────────────────────────"));
        Serial.println(F("  Kayıt2'yi başlatmak için 'Y' tuşuna basın."));
        Serial.println(F("  İptal için 'N' tuşuna basın."));
        Serial.print(F("  > "));
        
        durum = CK_X2_ONAY2_BEKLE;
      }
      break;
    
    // ───────────────────────────────────────────────────────────
    case CK_X2_ONAY2_BEKLE:
    // ───────────────────────────────────────────────────────────
      // AÇIKLAMA: Kullanıcıdan Y/N girişi bekliyoruz.
      //           Y = Kayıt2 başlat, N = İptal et
      
      if (Serial.available()) {
        char c = Serial.read();
        
        if (c == 'Y' || c == 'y') {
          Serial.println(F("Y\n"));
          
          // [ADIM 7/7] KAYIT2 BAŞLAT
          Serial.println(F("[ADIM 7/7] Kayıt2 başlatılıyor...\n"));
          kayitBaslat(yon2);
          
          durum = CK_KAYIT2_CALISIYOR;
        }
        else if (c == 'N' || c == 'n') {
          Serial.println(F("N\n"));
          Serial.println(F("✗ Çift kayıt iptal edildi!\n"));
          durum = CK_KAPALI;
        }
      }
      break;
    
    // ───────────────────────────────────────────────────────────
    case CK_KAYIT2_CALISIYOR:
    // ───────────────────────────────────────────────────────────
      // AÇIKLAMA: KayitModulu arka planı çalışıyor (kayitRun).
      
      if (kayitTamamlandiMi()) {
        Serial.println(F("\n✓ Kayıt2 tamamlandı!\n"));
        
        // ═════════════════════════════════════════════════════════
        // KAYIT2'Yİ KOPYALA
        // ═════════════════════════════════════════════════════════
        const KM_Sample* src = kayitVerileri();
        for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
          kayit2[i].enc = src[i].enc;
          kayit2[i].a0 = src[i].a0;
        }
        
        // ═════════════════════════════════════════════════════════
        // GLOBAL A0 MIN/MAX HESAPLA
        // ═════════════════════════════════════════════════════════
        // AÇIKLAMA: İki kayıttan EN KÜÇÜK ve EN BÜYÜK A0 değerlerini bul.
        //           Bu değerler Çift Oynatma'da Z encoder'ı ölçeklendirmek
        //           için kullanılır.
        
        globalA0Min = 1023;
        globalA0Max = 0;
        
        // Kayıt1'den min/max
        for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
          if (kayit1[i].a0 < globalA0Min) globalA0Min = kayit1[i].a0;
          if (kayit1[i].a0 > globalA0Max) globalA0Max = kayit1[i].a0;
        }
        
        // Kayıt2'den min/max
        for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
          if (kayit2[i].a0 < globalA0Min) globalA0Min = kayit2[i].a0;
          if (kayit2[i].a0 > globalA0Max) globalA0Max = kayit2[i].a0;
        }
        
        // ═════════════════════════════════════════════════════════
        // TAMAMLANDI RAPORU
        // ═════════════════════════════════════════════════════════
        Serial.println(F("\n╔════════════════════════════════════════════════╗"));
        Serial.println(F("║          ÇİFT KAYIT TAMAMLANDI! ✓              ║"));
        Serial.println(F("╚════════════════════════════════════════════════╝\n"));
        
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
    
    // ───────────────────────────────────────────────────────────
    case CK_TAMAMLANDI:
    // ───────────────────────────────────────────────────────────
      // Hiçbir şey yapma (bitti)
      return;
    
    // ───────────────────────────────────────────────────────────
    default:
    // ───────────────────────────────────────────────────────────
      Serial.println(F("✗ Bilinmeyen durum!"));
      durum = CK_KAPALI;
      break;
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

uint16_t ckGlobalA0Min() {
  return globalA0Min;
}

uint16_t ckGlobalA0Max() {
  return globalA0Max;
}

const CK_Sample* ckKayit1Verileri() {
  return kayit1;
}

const CK_Sample* ckKayit2Verileri() {
  return kayit2;
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
  Serial.println(F("\n[ÇİFT KAYIT] Acil durduruldu!"));
  
  // Motorları durdur
  moveToDurdur(MOTOR_X);
  moveToDurdur(MOTOR_B);
  
  // Kayıt modülünü durdur
  kayitDurdur();
  
  // Durumu sıfırla
  durum = CK_KAPALI;
  
  Serial.println();
}