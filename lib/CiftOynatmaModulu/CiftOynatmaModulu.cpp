// CiftOynatmaModulu.cpp - İKİ KAYDI SIRAYLA OYNATMA + GÜVENLİKLİ GEÇİŞ
#include "CiftOynatmaModulu.h"
#include "Config.h"
#include "CiftKayitModulu.h"
#include "OynatmaModulu.h"
#include "MoveTo.h"
#include "KayitModulu.h"

enum CODurum {
  CO_KAPALI = 0,
  CO_Z_SIFIRLAMA_HAZIRLIK,
  CO_Z_SIFIRLAMA_ONAY,
  CO_SIFIR_KONUMUNA_GIT,
  CO_SIFIR_ONAY,
  CO_K1_GECIS_Z_YUKARI,
  CO_K1_GECIS_BIG_X,
  CO_K1_GECIS_Z_ASAGI,
  CO_K1_OYNATILIYOR,
  CO_K2_GECIS_Z_YUKARI,
  CO_K2_GECIS_BIG_X,
  CO_K2_GECIS_Z_ASAGI,
  CO_K2_OYNATILIYOR,
  CO_TAMAMLANDI
};

static CODurum durum = CO_KAPALI;
static bool zSifirlandiMi = false;
static StepMotorEncoder* bigEnc = nullptr;
static StepMotorEncoder* xEnc = nullptr;
static StepMotorEncoder* zEnc = nullptr;
static long x1Hedef = 0;
static long x2Hedef = 0;
static uint16_t globalA0Min = 0;
static uint16_t globalA0Max = 0;
static long* bigFreqMinPtr = nullptr;
static long* bigFreqMaxPtr = nullptr;
static long* zEncMinPtr = nullptr;
static long* zEncMaxPtr = nullptr;

static long mapA0ToZ(uint16_t a0Val) {
  if (globalA0Max == globalA0Min) return *zEncMinPtr;
  int64_t num = (int64_t)(a0Val - globalA0Min) * (int64_t)(*zEncMaxPtr - *zEncMinPtr);
  int64_t den = (int64_t)(globalA0Max - globalA0Min);
  return *zEncMinPtr + (long)(num / den);
}

void coEncoderSetup(StepMotorEncoder* bigEncoder, StepMotorEncoder* xEncoder, StepMotorEncoder* zEncoder) {
  bigEnc = bigEncoder;
  xEnc = xEncoder;
  zEnc = zEncoder;
}

void coParametreSetup(long* bigFreqMin, long* bigFreqMax, long* zEncMin, long* zEncMax) {
  bigFreqMinPtr = bigFreqMin;
  bigFreqMaxPtr = bigFreqMax;
  zEncMinPtr = zEncMin;
  zEncMaxPtr = zEncMax;
}

void coBaslat(long x1Enc, long x2Enc) {
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║          ÇİFT OYNATMA BAŞLATILIYOR             ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
  
  if (!ckTamamlandiMi()) {
    Serial.println(F("✗ Hata: Çift kayıt tamamlanmamış!"));
    return;
  }
  
  if (bigEnc == nullptr || xEnc == nullptr || zEnc == nullptr) {
    Serial.println(F("✗ Hata: Encoder'lar ayarlanmamış!\n"));
    return;
  }
  
  if (bigFreqMinPtr == nullptr || zEncMaxPtr == nullptr) {
    Serial.println(F("✗ Hata: Parametreler ayarlanmamış!\n"));
    return;
  }
  
  x1Hedef = x1Enc;
  x2Hedef = x2Enc;
  globalA0Min = ckGlobalA0Min();
  globalA0Max = ckGlobalA0Max();
  *zEncMaxPtr = (long)((globalA0Max - globalA0Min) * 16000L / 1023L);
  
  Serial.print(F("  X1 Pozisyonu   : "));
  Serial.println(x1Hedef);
  Serial.print(F("  X2 Pozisyonu   : "));
  Serial.println(x2Hedef);
  Serial.print(F("  Global A0_min  : "));
  Serial.println(globalA0Min);
  Serial.print(F("  Global A0_max  : "));
  Serial.println(globalA0Max);
  Serial.print(F("  Z Enc Max      : "));
  Serial.println(*zEncMaxPtr);
  Serial.println();
  
  if (!zSifirlandiMi) {
    const CK_Sample* kayit1 = ckKayit1Verileri();
    const CK_Sample* kayit2 = ckKayit2Verileri();
    long bigA0Min = 0;
    long xA0Min = x1Hedef;
    
    for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
      if (kayit1[i].a0 == globalA0Min) {
        bigA0Min = kayit1[i].enc;
        xA0Min = x1Hedef;
        break;
      }
    }
    
    if (bigA0Min == 0) {
      for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
        if (kayit2[i].a0 == globalA0Min) {
          bigA0Min = kayit2[i].enc;
          xA0Min = x2Hedef;
          break;
        }
      }
    }
    
    Serial.println(F("[ADIM 1] Z Sıfırlama Hazırlığı (İlk Depo)"));
    Serial.println(F("───────────────────────────────────────────────"));
    Serial.print(F("  BIG & X → A0_min konumuna gidiyor...\n"));
    Serial.print(F("  BIG: "));
    Serial.print(bigEnc->getPosition());
    Serial.print(F(" → "));
    Serial.println(bigA0Min);
    Serial.print(F("  X  : "));
    Serial.print(xEnc->getPosition());
    Serial.print(F(" → "));
    Serial.println(xA0Min);
    Serial.println();
    
    moveTo(MOTOR_B, bigA0Min, 100);
    moveTo(MOTOR_X, xA0Min, 100);
    durum = CO_Z_SIFIRLAMA_HAZIRLIK;
  }
  else {
    const CK_Sample* kayit1 = ckKayit1Verileri();
    long bigHedef = kayit1[0].enc;
    
    Serial.println(F("[ADIM 1] Başlangıç Konumuna Git (Yeni Depo)"));
    Serial.println(F("───────────────────────────────────────────────"));
    Serial.print(F("  BIG: "));
    Serial.print(bigEnc->getPosition());
    Serial.print(F(" → "));
    Serial.println(bigHedef);
    Serial.print(F("  X  : "));
    Serial.print(xEnc->getPosition());
    Serial.print(F(" → "));
    Serial.println(x1Hedef);
    Serial.println();
    
    moveTo(MOTOR_B, bigHedef, 100);
    moveTo(MOTOR_X, x1Hedef, 100);
    durum = CO_SIFIR_KONUMUNA_GIT;
  }
}

void coRun() {
  switch (durum) {
    case CO_KAPALI:
      return;
    
    case CO_Z_SIFIRLAMA_HAZIRLIK:
      if (!moveToAktifMi(MOTOR_B) && !moveToAktifMi(MOTOR_X)) {
        Serial.println(F("✓ A0_min konumuna ulaşıldı!\n"));
        Serial.print(F("  BIG: "));
        Serial.println(bigEnc->getPosition());
        Serial.print(F("  X  : "));
        Serial.println(xEnc->getPosition());
        Serial.println(F("\n[ADIM 2] Manuel Torch Sürme"));
        Serial.println(F("───────────────────────────────────────────────"));
        Serial.println(F("  Bu nokta en düşük A0 değerinin olduğu yer."));
        Serial.println(F("  Torcu manuel olarak (M, P komutları ile)"));
        Serial.println(F("  yüzeye yaklaştırın.\n"));
        Serial.println(F("  Torch yüzeye değdiğinde 'Y' tuşlayın."));
        Serial.println(F("  (Z encoder sıfırlanacak: Z=0)"));
        Serial.println(F("───────────────────────────────────────────────"));
        Serial.print(F("  > Hazır mısınız? (Y/N): "));
        durum = CO_Z_SIFIRLAMA_ONAY;
      }
      break;
    
    case CO_Z_SIFIRLAMA_ONAY:
      if (Serial.available()) {
        char c = Serial.read();
        if (c == 'Y' || c == 'y') {
          Serial.println(F("Y\n"));
          zEnc->reset();
          zSifirlandiMi = true;
          Serial.println(F("✓ Z encoder sıfırlandı! (Z = 0)"));
          Serial.println(F("  Bu nokta artık kalıcı referans.\n"));
          Serial.println(F("[ADIM 3] Kayıt1 Geçişi: Z Yukarı"));
          Serial.println(F("───────────────────────────────────────────────"));
          long zMaxPos = mapA0ToZ(globalA0Max);
          Serial.print(F("  Z: 0 → "));
          Serial.print(zMaxPos);
          Serial.println(F(" (Güvenlik - Yukarı)"));
          moveTo(MOTOR_Z, zMaxPos, 2000);
          durum = CO_K1_GECIS_Z_YUKARI;
        }
        else if (c == 'N' || c == 'n') {
          Serial.println(F("N\n"));
          Serial.println(F("✗ Çift oynatma iptal edildi!\n"));
          durum = CO_KAPALI;
        }
      }
      break;
    
    case CO_SIFIR_KONUMUNA_GIT:
      if (!moveToAktifMi(MOTOR_B) && !moveToAktifMi(MOTOR_X)) {
        Serial.println(F("✓ Başlangıç konumuna ulaşıldı!\n"));
        Serial.print(F("  BIG: "));
        Serial.println(bigEnc->getPosition());
        Serial.print(F("  X  : "));
        Serial.println(xEnc->getPosition());
        Serial.println(F("\n[ADIM 2] Konum Onayı"));
        Serial.println(F("───────────────────────────────────────────────"));
        Serial.println(F("  Motorlar Kayıt1'in başlangıç konumunda."));
        Serial.println(F("  Görsel kontrol yapın.\n"));
        Serial.println(F("  Konum doğru mu? (Y/N)"));
        Serial.println(F("───────────────────────────────────────────────"));
        Serial.print(F("  > "));
        durum = CO_SIFIR_ONAY;
      }
      break;
    
    case CO_SIFIR_ONAY:
      if (Serial.available()) {
        char c = Serial.read();
        if (c == 'Y' || c == 'y') {
          Serial.println(F("Y\n"));
          Serial.println(F("✓ Onaylandı! Oynatma başlıyor...\n"));
          Serial.println(F("[ADIM 3] Kayıt1 Geçişi: Z Aşağı"));
          Serial.println(F("───────────────────────────────────────────────"));
          const CK_Sample* kayit1 = ckKayit1Verileri();
          long zHedef = mapA0ToZ(kayit1[0].a0);
          Serial.print(F("  Z: "));
          Serial.print(zEnc->getPosition());
          Serial.print(F(" → "));
          Serial.print(zHedef);
          Serial.print(F(" (A0="));
          Serial.print(kayit1[0].a0);
          Serial.println(F(")"));
          moveTo(MOTOR_Z, zHedef, 2000);
          durum = CO_K1_GECIS_Z_ASAGI;
        }
        else if (c == 'N' || c == 'n') {
          Serial.println(F("N\n"));
          Serial.println(F("✗ Çift oynatma iptal edildi!\n"));
          durum = CO_KAPALI;
        }
      }
      break;
    
    case CO_K1_GECIS_Z_YUKARI:
      if (moveToBittiMi(MOTOR_Z)) {
        Serial.print(F("  ✓ Z yukarıda: "));
        Serial.println(zEnc->getPosition());
        Serial.println(F("\n[ADIM 4] Kayıt1 Geçişi: BIG & X Hareket"));
        Serial.println(F("───────────────────────────────────────────────"));
        const CK_Sample* kayit1 = ckKayit1Verileri();
        long bigHedef = kayit1[0].enc;
        Serial.print(F("  BIG: "));
        Serial.print(bigEnc->getPosition());
        Serial.print(F(" → "));
        Serial.println(bigHedef);
        Serial.print(F("  X  : "));
        Serial.print(xEnc->getPosition());
        Serial.print(F(" → "));
        Serial.println(x1Hedef);
        Serial.println();
        moveTo(MOTOR_B, bigHedef, 100);
        moveTo(MOTOR_X, x1Hedef, 100);
        durum = CO_K1_GECIS_BIG_X;
      }
      break;
    
    case CO_K1_GECIS_BIG_X:
      if (!moveToAktifMi(MOTOR_B) && !moveToAktifMi(MOTOR_X)) {
        Serial.println(F("  ✓ Kayıt1[0] konumunda"));
        Serial.print(F("    BIG: "));
        Serial.println(bigEnc->getPosition());
        Serial.print(F("    X  : "));
        Serial.println(xEnc->getPosition());
        Serial.println(F("\n[ADIM 5] Kayıt1 Geçişi: Z Aşağı"));
        Serial.println(F("───────────────────────────────────────────────"));
        const CK_Sample* kayit1 = ckKayit1Verileri();
        long zHedef = mapA0ToZ(kayit1[0].a0);
        Serial.print(F("  Z: "));
        Serial.print(zEnc->getPosition());
        Serial.print(F(" → "));
        Serial.print(zHedef);
        Serial.print(F(" (A0="));
        Serial.print(kayit1[0].a0);
        Serial.println(F(")"));
        moveTo(MOTOR_Z, zHedef, 2000);
        durum = CO_K1_GECIS_Z_ASAGI;
      }
      break;
    
    case CO_K1_GECIS_Z_ASAGI:
      if (moveToBittiMi(MOTOR_Z)) {
        Serial.print(F("  ✓ Z başlangıç konumunda: "));
        Serial.println(zEnc->getPosition());
        Serial.println(F("\n[ADIM 6] Kayıt1 Oynatma"));
        Serial.println(F("───────────────────────────────────────────────"));
        const CK_Sample* kayit1 = ckKayit1Verileri();
        KM_Sample* oynatmaBuffer = kayitVerileriDuzenle();
        for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
          oynatmaBuffer[i].enc = kayit1[i].enc;
          oynatmaBuffer[i].a0 = kayit1[i].a0;
        }
        Serial.println(F("  ✓ Kayıt1 yüklendi!"));
        Serial.println(F("  → oynatmaBaslatGercek() çağrılıyor...\n"));
        oynatmaBaslatGercek();
        durum = CO_K1_OYNATILIYOR;
      }
      break;
    
    case CO_K1_OYNATILIYOR:
      if (oynatmaTamamlandiMi()) {
        Serial.println(F("\n✓ Kayıt1 oynatma tamamlandı!\n"));
        Serial.println(F("[ADIM 7] Kayıt2 Geçişi: Z Yukarı"));
        Serial.println(F("───────────────────────────────────────────────"));
        long zMaxPos = mapA0ToZ(globalA0Max);
        Serial.print(F("  Z: "));
        Serial.print(zEnc->getPosition());
        Serial.print(F(" → "));
        Serial.print(zMaxPos);
        Serial.println(F(" (Güvenlik - Yukarı)"));
        moveTo(MOTOR_Z, zMaxPos, 2000);
        durum = CO_K2_GECIS_Z_YUKARI;
      }
      break;
    
    case CO_K2_GECIS_Z_YUKARI:
      if (moveToBittiMi(MOTOR_Z)) {
        Serial.print(F("  ✓ Z yukarıda: "));
        Serial.println(zEnc->getPosition());
        Serial.println(F("\n[ADIM 8] Kayıt2 Geçişi: BIG & X Hareket"));
        Serial.println(F("───────────────────────────────────────────────"));
        const CK_Sample* kayit2 = ckKayit2Verileri();
        long bigHedef = kayit2[0].enc;
        Serial.print(F("  BIG: "));
        Serial.print(bigEnc->getPosition());
        Serial.print(F(" → "));
        Serial.println(bigHedef);
        Serial.print(F("  X  : "));
        Serial.print(xEnc->getPosition());
        Serial.print(F(" → "));
        Serial.println(x2Hedef);
        Serial.println();
        moveTo(MOTOR_B, bigHedef, 100);
        moveTo(MOTOR_X, x2Hedef, 100);
        durum = CO_K2_GECIS_BIG_X;
      }
      break;
    
    case CO_K2_GECIS_BIG_X:
      if (!moveToAktifMi(MOTOR_B) && !moveToAktifMi(MOTOR_X)) {
        Serial.println(F("  ✓ Kayıt2[0] konumunda"));
        Serial.print(F("    BIG: "));
        Serial.println(bigEnc->getPosition());
        Serial.print(F("    X  : "));
        Serial.println(xEnc->getPosition());
        Serial.println(F("\n[ADIM 9] Kayıt2 Geçişi: Z Aşağı"));
        Serial.println(F("───────────────────────────────────────────────"));
        const CK_Sample* kayit2 = ckKayit2Verileri();
        long zHedef = mapA0ToZ(kayit2[0].a0);
        Serial.print(F("  Z: "));
        Serial.print(zEnc->getPosition());
        Serial.print(F(" → "));
        Serial.print(zHedef);
        Serial.print(F(" (A0="));
        Serial.print(kayit2[0].a0);
        Serial.println(F(")"));
        moveTo(MOTOR_Z, zHedef, 2000);
        durum = CO_K2_GECIS_Z_ASAGI;
      }
      break;
    
    case CO_K2_GECIS_Z_ASAGI:
      if (moveToBittiMi(MOTOR_Z)) {
        Serial.print(F("  ✓ Z başlangıç konumunda: "));
        Serial.println(zEnc->getPosition());
        Serial.println(F("\n[ADIM 10] Kayıt2 Oynatma"));
        Serial.println(F("───────────────────────────────────────────────"));
        const CK_Sample* kayit2 = ckKayit2Verileri();
        KM_Sample* oynatmaBuffer = kayitVerileriDuzenle();
        for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
          oynatmaBuffer[i].enc = kayit2[i].enc;
          oynatmaBuffer[i].a0 = kayit2[i].a0;
        }
        Serial.println(F("  ✓ Kayıt2 yüklendi!"));
        Serial.println(F("  → oynatmaBaslatGercek() çağrılıyor...\n"));
        oynatmaBaslatGercek();
        durum = CO_K2_OYNATILIYOR;
      }
      break;
    
    case CO_K2_OYNATILIYOR:
      if (oynatmaTamamlandiMi()) {
        Serial.println(F("\n✓ Kayıt2 oynatma tamamlandı!\n"));
        Serial.println(F("\n╔════════════════════════════════════════════════╗"));
        Serial.println(F("║         ÇİFT OYNATMA TAMAMLANDI! ✓             ║"));
        Serial.println(F("╚════════════════════════════════════════════════╝\n"));
        Serial.println(F("─────────────────────────────────────────────────"));
        Serial.println(F("SONUÇ:"));
        Serial.print(F("  Kayıt1 (X1="));
        Serial.print(x1Hedef);
        Serial.println(F(") → Oynatıldı ✓"));
        Serial.print(F("  Kayıt2 (X2="));
        Serial.print(x2Hedef);
        Serial.println(F(") → Oynatıldı ✓"));
        Serial.println(F("─────────────────────────────────────────────────\n"));
        Serial.println(F("✓ Tekrar oynatmak için 'CO' tuşlayın.\n"));
        durum = CO_TAMAMLANDI;
      }
      break;
    
    case CO_TAMAMLANDI:
      return;
    
    default:
      Serial.println(F("✗ Bilinmeyen durum!"));
      durum = CO_KAPALI;
      break;
  }
}

void coZSifirlamaReset() {
  zSifirlandiMi = false;
  Serial.println(F("[ÇİFT OYNATMA] Z sıfırlama bayrağı sıfırlandı."));
  Serial.println(F("  Bir sonraki CO komutunda Z sıfırlama yapılacak.\n"));
}

bool coAktifMi() {
  return (durum != CO_KAPALI && durum != CO_TAMAMLANDI);
}

bool coTamamlandiMi() {
  return (durum == CO_TAMAMLANDI);
}

uint8_t coAsama() {
  if (durum == CO_KAPALI) return 0;
  if (durum <= CO_Z_SIFIRLAMA_ONAY) return 1;
  if (durum <= CO_K1_OYNATILIYOR) return 2;
  if (durum <= CO_K2_OYNATILIYOR) return 3;
  if (durum == CO_TAMAMLANDI) return 4;
  return 0;
}

void coDurdur() {
  Serial.println(F("\n[ÇİFT OYNATMA] Acil durduruldu!"));
  moveToDurdur(MOTOR_X);
  moveToDurdur(MOTOR_B);
  moveToDurdur(MOTOR_Z);
  oynatmaDurdur();
  durum = CO_KAPALI;
  Serial.println();
}