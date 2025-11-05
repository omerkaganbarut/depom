// CiftOynatmaModulu.cpp - v8.0 Y/N ONAY SİSTEMİ
// ═══════════════════════════════════════════════════════════════
// ✅ DÜZELTMELER:
// 1. Kayıt1 oynatılmadan önce Y/N onayı
// 2. Kayıt2 oynatılmadan önce Y/N onayı
// 3. Z sıfırlama MANUEL TORCH SÜRME (Y/N ile)
// 4. Döngü hataları giderildi (moveTo tek seferlik)
// 5. Global A0_min pozisyonunda kullanıcı torch'u sürebilir
// ═══════════════════════════════════════════════════════════════

#include "CiftOynatmaModulu.h"
#include "CiftKayitModulu.h"
#include "OynatmaModulu.h"
#include "MoveTo.h"
#include "PulseAt.h"
#include "A0Filtre.h"
#include "Config.h"
#include <Arduino.h>
#include "MoveSalinim.h"

// ═══════════════════════════════════════════════════════════════
// DURUMLAR
// ═══════════════════════════════════════════════════════════════
enum CiftOynatmaDurum {
  CO_KAPALI = 0,
  
  // Z SIFIRLAMA (MANUEL TORCH SÜRME)
  CO_Z_SIFIRLAMA_A0MIN_BASLA,
  CO_Z_SIFIRLAMA_A0MIN_BEKLE,
  CO_Z_SIFIRLAMA_KONTROL,      // ← Y/N bekle (kullanıcı manuel sürer)
  CO_Z_SIFIRLAMA_YUKARI_BASLA,
  CO_Z_SIFIRLAMA_YUKARI_BEKLE,
  
  // KAYIT1 GEÇİŞİ
  CO_K1_GECIS_BASLA,
  CO_K1_GECIS_BEKLE,
  CO_K1_Z_ASAGI_BASLA,
  CO_K1_Z_ASAGI_BEKLE,
  
  // KAYIT1 OYNATMA
  CO_K1_OYNAT_ONAY_BEKLE,  // ← YENİ: Kayıt1 oynatma onayı
  CO_K1_OYNAT,
  
  // KAYIT2 GEÇİŞİ
  CO_K2_Z_YUKARI_BASLA,
  CO_K2_Z_YUKARI_BEKLE,
  CO_K2_XB_BASLA,
  CO_K2_XB_BEKLE,
  CO_K2_Z_ASAGI_BASLA,
  CO_K2_Z_ASAGI_BEKLE,
  
  // KAYIT2 OYNATMA
  CO_K2_OYNAT_ONAY_BEKLE,  // ← YENİ: Kayıt2 oynatma onayı
  CO_K2_OYNAT,
  
  CO_BITTI
};

// ═══════════════════════════════════════════════════════════════
// GLOBAL DEĞİŞKENLER
// ═══════════════════════════════════════════════════════════════
static CiftOynatmaDurum durum = CO_KAPALI;

static StepMotorEncoder* bigEnc = nullptr;
static StepMotorEncoder* xEnc = nullptr;
static StepMotorEncoder* zEnc = nullptr;

static long* bigFreqMin_ptr = nullptr;
static long* bigFreqMax_ptr = nullptr;
static long* zEncMin_ptr = nullptr;
static long* zEncMax_ptr = nullptr;

static long x1Hedef = 0;
static long x2Hedef = 0;

static bool zSifirlamaTamamlandi = false;

// A0_MIN pozisyonu
static long bigPosAtA0Min = 0;
static long xPosAtA0Min = 0;

// EXTERN: GLOBAL A0 MIN/MAX
extern uint16_t globalA0Min;
extern uint16_t globalA0Max;

// ═══════════════════════════════════════════════════════════════
// ENCODER SETUP
// ═══════════════════════════════════════════════════════════════
void coEncoderSetup(StepMotorEncoder* bigEncoder, 
                    StepMotorEncoder* xEncoder,
                    StepMotorEncoder* zEncoder) {
  bigEnc = bigEncoder;
  xEnc = xEncoder;
  zEnc = zEncoder;
}

// ═══════════════════════════════════════════════════════════════
// PARAMETRE SETUP
// ═══════════════════════════════════════════════════════════════
void coParametreSetup(long* bigFreqMin, long* bigFreqMax, 
                      long* zEncMin, long* zEncMax) {
  bigFreqMin_ptr = bigFreqMin;
  bigFreqMax_ptr = bigFreqMax;
  zEncMin_ptr = zEncMin;
  zEncMax_ptr = zEncMax;
}

// ═══════════════════════════════════════════════════════════════
// HELPER: GLOBAL A0 ARALIĞINA GÖRE Z MAX HESAPLA
// ═══════════════════════════════════════════════════════════════
static inline long hesaplaZMax() {
  // Global A0 ARALIĞININ fiziksel Z karşılığı
  // Örnek: globalA0Min=300, globalA0Max=600
  //        Aralık = 600-300 = 300
  //        Z Max = (300/1023) × 160000 = 46,950
  
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
// HELPER: A0 → Z ENCODER MAPPING
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
// HELPER: A0_MIN POZİSYONUNU BUL
// ═══════════════════════════════════════════════════════════════
static void hesaplaA0MinPozisyonu() {
  Serial.println(F("\n[CO] Sıfırlama pozisyonu aranıyor..."));
  Serial.print(F("  Global A0 Min: ")); Serial.println(globalA0Min);
  
  bigPosAtA0Min = __LONG_MAX__;
  xPosAtA0Min = __LONG_MAX__;
  
  // Kayıt1'de ara
  for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
    if (kayit1[i].a0 == globalA0Min) {
      bigPosAtA0Min = kayit1[i].enc;
      xPosAtA0Min = x1Hedef;
      
      Serial.print(F("  ✓ Kayıt1["));
      Serial.print(i);
      Serial.print(F("]: bigEnc="));
      Serial.print(bigPosAtA0Min);
      Serial.print(F(", xEnc="));
      Serial.println(xPosAtA0Min);
      break;
    }
  }
  
  // Kayıt2'de ara
  if (bigPosAtA0Min == __LONG_MAX__) {
    for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
      if (kayit2[i].a0 == globalA0Min) {
        bigPosAtA0Min = kayit2[i].enc;
        xPosAtA0Min = x2Hedef;
        
        Serial.print(F("  ✓ Kayıt2["));
        Serial.print(i);
        Serial.print(F("]: bigEnc="));
        Serial.print(bigPosAtA0Min);
        Serial.print(F(", xEnc="));
        Serial.println(xPosAtA0Min);
        break;
      }
    }
  }
  
  if (bigPosAtA0Min == __LONG_MAX__) {
    Serial.println(F("✗ HATA: globalA0Min pozisyonu bulunamadı!"));
    durum = CO_KAPALI;
    return;
  }
  
  Serial.println(F("[CO] Sıfırlama pozisyonu bulundu!"));
}

// ═══════════════════════════════════════════════════════════════
// BAŞLATMA
// ═══════════════════════════════════════════════════════════════
void coBaslat(long x1Enc, long x2Enc) {
  x1Hedef = x1Enc;
  x2Hedef = x2Enc;
  
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║          ÇİFT OYNATMA BAŞLATILIYOR             ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
  
  hesaplaA0MinPozisyonu();
  
  if (!zSifirlamaTamamlandi) {
    Serial.println(F("[CO] Z sıfırlama başlıyor (MANUEL TORCH SÜRME)..."));
    durum = CO_Z_SIFIRLAMA_A0MIN_BASLA;
  } else {
    Serial.println(F("[CO] Z zaten sıfır. Kayıt1 geçişi başlıyor..."));
    durum = CO_K1_GECIS_BASLA;
  }
}

// ═══════════════════════════════════════════════════════════════
// ÇALIŞMA DÖNGÜSÜ
// ═══════════════════════════════════════════════════════════════
void coRun() {
  if (!bigEnc || !xEnc || !zEnc) return;
  if (durum == CO_KAPALI || durum == CO_BITTI) return;
  
  switch (durum) {
    
    // ═══════════════════════════════════════════════════════════
    // Z SIFIRLAMA (MANUEL TORCH SÜRME)
    // ═══════════════════════════════════════════════════════════
    
    case CO_Z_SIFIRLAMA_A0MIN_BASLA:
    {
      Serial.println(F("[CO] Z Sıfırlama: Global A0_min pozisyonuna gidiliyor..."));
      Serial.print(F("  BIG: ")); Serial.println(bigPosAtA0Min);
      Serial.print(F("  X  : ")); Serial.println(xPosAtA0Min);
      
      moveTo(MOTOR_B, bigPosAtA0Min, 200,false);
      moveTo(MOTOR_X, xPosAtA0Min, 10000,false);
      //moveTo(MOTOR_Z, 100000, 10000); //DENEME
      
      durum = CO_Z_SIFIRLAMA_A0MIN_BEKLE;
    }
    break;
    
    case CO_Z_SIFIRLAMA_A0MIN_BEKLE:
    {
      // ✅ Motorlar durdu mu kontrol et (aktif değil mi?)
      bool xDurdu = !moveToAktifMi(MOTOR_X);
      bool bigDurdu = !moveToAktifMi(MOTOR_B);
      //bool zDurdu = !moveToAktifMi(MOTOR_Z);
      
      if (xDurdu && bigDurdu ) {
        pulseAtDurdur(MOTOR_X);
        pulseAtDurdur(MOTOR_B);
        //pulseAtDurdur(MOTOR_Z);
        
        Serial.println(F("[CO] ✓ Global A0_min pozisyonunda!"));
        Serial.println(F("─────────────────────────────────────────────"));
        Serial.println(F("  🔧 MANUEL TORCH SÜRME MODU"));
        Serial.println(F("─────────────────────────────────────────────"));
        Serial.println(F("  Bu konumda torch'u Z ekseninde yere değdirin."));
        Serial.println(F("  Main menüden Z motor komutlarını kullanabilirsiniz."));
        Serial.println();
        Serial.println(F("  Torch yere değdiğinde:"));
        Serial.println(F("    Y → Z encoder'ı sıfırla ve devam et"));
        Serial.println(F("    N → İptal"));
        Serial.print(F("  > "));
        
        durum = CO_Z_SIFIRLAMA_KONTROL;
      }
    }
    break;
    
    case CO_Z_SIFIRLAMA_KONTROL:
    {
      // Y/N bekle (main.cpp'den manuel sürme sırasında)
      if (Serial.available() > 0) {
        char c = Serial.peek();
        
        if (c == 'Y' || c == 'y') {
          Serial.read();
          Serial.println(F("Y\n"));
          
          Serial.println(F("  ✓ Z encoder sıfırlanıyor..."));
          
          zEnc->reset();  // ← Artık Z=0 bu A0_min noktası
          
          Serial.print(F("  ✓ Z=0 ayarlandı! (Mevcut Z: "));
          Serial.print(zEnc->getPosition());
          Serial.println(F(")"));
          
          Serial.println(F("  Z yukarı çıkıyor..."));
          
          durum = CO_Z_SIFIRLAMA_YUKARI_BASLA;
        }
        else if (c == 'N' || c == 'n') {
          Serial.read();
          Serial.println(F("N\n"));
          
          Serial.println(F("  ✗ Çift oynatma iptal edildi!"));
          
          durum = CO_KAPALI;
        }
      }
    }
    break;
    
    case CO_Z_SIFIRLAMA_YUKARI_BASLA:
    {
      // ✅ Global A0 aralığına göre Z max hesapla
      long zHedef = hesaplaZMax();
      
      Serial.print(F("  Global A0 Min: ")); Serial.println(globalA0Min);
      Serial.print(F("  Global A0 Max: ")); Serial.println(globalA0Max);
      Serial.print(F("  A0 Aralığı: ")); Serial.println(globalA0Max - globalA0Min);
      Serial.print(F("  Hesaplanan Z Max: ")); Serial.println(zHedef);
      
      moveTo(MOTOR_Z, zHedef, 10000,false);
      
      durum = CO_Z_SIFIRLAMA_YUKARI_BEKLE;
    }
    break;
    
    case CO_Z_SIFIRLAMA_YUKARI_BEKLE:
    {
      if (!moveToAktifMi(MOTOR_Z)) {
        pulseAtDurdur(MOTOR_Z);
        
        Serial.println(F("[CO] ✓ Z sıfırlama tamamlandı!"));
        zSifirlamaTamamlandi = true;
        
        durum = CO_K1_GECIS_BASLA;
      }
    }
    break;
    
    // ═══════════════════════════════════════════════════════════
    // KAYIT1 GEÇİŞİ
    // ═══════════════════════════════════════════════════════════
    
    case CO_K1_GECIS_BASLA:
    {
      Serial.println(F("[CO] Kayıt1 Geçişi: Z yukarı → BIG+X → Kayıt1[0]"));
      
      // ✅ Z yukarı (global A0 max'a göre)
      long zHedef = hesaplaZMax();
      moveTo(MOTOR_Z, zHedef, 10000,false);
      
      // BIG+X → Kayıt1[0]
      moveTo(MOTOR_B, kayit1[0].enc, 200,false);
      moveTo(MOTOR_X, x1Hedef, 10000,false);
      
      durum = CO_K1_GECIS_BEKLE;
    }
    break;
    
    case CO_K1_GECIS_BEKLE:
    {
      bool zDurdu = !moveToAktifMi(MOTOR_Z);
      bool xDurdu = !moveToAktifMi(MOTOR_X);
      bool bigDurdu = !moveToAktifMi(MOTOR_B);
      
      if (zDurdu && xDurdu && bigDurdu) {
        pulseAtDurdur(MOTOR_Z);
        pulseAtDurdur(MOTOR_X);
        pulseAtDurdur(MOTOR_B);
        
        Serial.println(F("[CO] ✓ Kayıt1[0] pozisyonu. Z iniyor..."));
        
        durum = CO_K1_Z_ASAGI_BASLA;
      }
    }
    break;
    
    case CO_K1_Z_ASAGI_BASLA:
    {
      // Z → Kayıt1[0].a0
      uint16_t a0Hedef = kayit1[0].a0;
      long zHedef = mapA0ToZEnc(a0Hedef);
      
      Serial.print(F("  Z → ")); Serial.println(zHedef);
      
      moveTo(MOTOR_Z, zHedef, 3000,false);
      
      durum = CO_K1_Z_ASAGI_BEKLE;
    }
    break;
    
    case CO_K1_Z_ASAGI_BEKLE:
    {
      if (!moveToAktifMi(MOTOR_Z)) {
        pulseAtDurdur(MOTOR_Z);
        
        Serial.println(F("[CO] ✓ Kayıt1 başlangıç pozisyonu hazır!"));
        Serial.println(F("\n[CO] ╔════════════════════════════════════════╗"));
        Serial.println(F("[CO] ║  Kayıt1 oynatılsın mı? (Y/N)         ║"));
        Serial.println(F("[CO] ╚════════════════════════════════════════╝"));
        
        durum = CO_K1_OYNAT_ONAY_BEKLE;
      }
    }
    break;
    
    // ═══════════════════════════════════════════════════════════
    // KAYIT1 OYNATMA ONAYI
    // ═══════════════════════════════════════════════════════════
    
    case CO_K1_OYNAT_ONAY_BEKLE:
    {
      if (Serial.available() > 0) {
        char c = Serial.peek();
        
        if (c == 'Y' || c == 'y') {
          Serial.read();
          Serial.println(F("Y\n"));
          Serial.println(F("[CO] ✓ Kayıt1 oynatılıyor..."));
          
          // ✅ Kayıt1'i OynatmaModulu'ne ver
          oynatmaBaslatKayit(kayit1, KAYIT_ORNEK_SAYISI);
          msBaslat(600, 10000);
          
          durum = CO_K1_OYNAT;
        }
        else if (c == 'N' || c == 'n') {
          Serial.read();
          Serial.println(F("N\n"));
          Serial.println(F("[CO] ✗ Çift oynatma iptal edildi!"));
          
          durum = CO_KAPALI;
        }
      }
    }
    break;
    
    // ═══════════════════════════════════════════════════════════
    // KAYIT1 OYNATMA
    // ═══════════════════════════════════════════════════════════
    
    case CO_K1_OYNAT:
    {
      if (oynatmaTamamlandiMi()) {
        Serial.println(F("[CO] ✓ Kayıt1 tamamlandı!"));
        durum = CO_K2_Z_YUKARI_BASLA;
      }
    }
    break;
    
    // ═══════════════════════════════════════════════════════════
    // KAYIT2 GEÇİŞİ
    // ═══════════════════════════════════════════════════════════
    
    case CO_K2_Z_YUKARI_BASLA:
    {
      Serial.println(F("[CO] Kayıt2 Geçişi: Z yukarı"));
      
      // ✅ Z yukarı (global A0 max'a göre)
      long zHedef = hesaplaZMax();
      moveTo(MOTOR_Z, zHedef, 10000,false);
      
      durum = CO_K2_Z_YUKARI_BEKLE;
    }
    break;
    
    case CO_K2_Z_YUKARI_BEKLE:
    {
      if (!moveToAktifMi(MOTOR_Z)) {
        pulseAtDurdur(MOTOR_Z);
        
        Serial.println(F("[CO] ✓ Z yukarıda."));
        
        durum = CO_K2_XB_BASLA;
      }
    }
    break;
    
    case CO_K2_XB_BASLA:
    {
      Serial.println(F("[CO] BIG+X → Kayıt2[0]"));
      
      moveTo(MOTOR_B, kayit2[0].enc, 200,false);
      moveTo(MOTOR_X, x2Hedef, 10000,false);
      
      durum = CO_K2_XB_BEKLE;
    }
    break;
    
    case CO_K2_XB_BEKLE:
    {
      bool xDurdu = !moveToAktifMi(MOTOR_X);
      bool bigDurdu = !moveToAktifMi(MOTOR_B);
      
      if (xDurdu && bigDurdu) {
        pulseAtDurdur(MOTOR_X);
        pulseAtDurdur(MOTOR_B);
        
        Serial.println(F("[CO] ✓ Kayıt2[0] pozisyonu. Z iniyor..."));
        
        durum = CO_K2_Z_ASAGI_BASLA;
      }
    }
    break;
    
    case CO_K2_Z_ASAGI_BASLA:
    {
      // Z → Kayıt2[0].a0
      uint16_t a0Hedef = kayit2[0].a0;
      long zHedef = mapA0ToZEnc(a0Hedef);
      
      Serial.print(F("  Z → ")); Serial.println(zHedef);
      
      moveTo(MOTOR_Z, zHedef, 5000,false);
      
      durum = CO_K2_Z_ASAGI_BEKLE;
    }
    break;
    
    case CO_K2_Z_ASAGI_BEKLE:
    {
      if (!moveToAktifMi(MOTOR_Z)) {
        pulseAtDurdur(MOTOR_Z);
        
        Serial.println(F("[CO] ✓ Kayıt2 başlangıç pozisyonu hazır!"));
        Serial.println(F("\n[CO] ╔════════════════════════════════════════╗"));
        Serial.println(F("[CO] ║  Kayıt2 oynatılsın mı? (Y/N)         ║"));
        Serial.println(F("[CO] ╚════════════════════════════════════════╝"));
        
        durum = CO_K2_OYNAT_ONAY_BEKLE;
      }
    }
    break;
    
    // ═══════════════════════════════════════════════════════════
    // KAYIT2 OYNATMA ONAYI
    // ═══════════════════════════════════════════════════════════
    
    case CO_K2_OYNAT_ONAY_BEKLE:
    {
      if (Serial.available() > 0) {
        char c = Serial.peek();
        
        if (c == 'Y' || c == 'y') {
          Serial.read();
          Serial.println(F("Y\n"));
          Serial.println(F("[CO] ✓ Kayıt2 oynatılıyor..."));
          
          // ✅ Kayıt2'yi OynatmaModulu'ne ver
          oynatmaBaslatKayit(kayit2, KAYIT_ORNEK_SAYISI);
          msBaslat(600, 10000);
          
          durum = CO_K2_OYNAT;
        }
        else if (c == 'N' || c == 'n') {
          Serial.read();
          Serial.println(F("N\n"));
          Serial.println(F("[CO] ✗ Çift oynatma iptal edildi!"));
          
          durum = CO_KAPALI;
        }
      }
    }
    break;
    
    // ═══════════════════════════════════════════════════════════
    // KAYIT2 OYNATMA
    // ═══════════════════════════════════════════════════════════
    
    case CO_K2_OYNAT:
    {
      if (oynatmaTamamlandiMi()) {
        Serial.println(F("[CO] ✓ Kayıt2 tamamlandı!"));
        Serial.println(F("\n╔════════════════════════════════════════════════╗"));
        Serial.println(F("║       ÇİFT OYNATMA TAMAMLANDI! ✓               ║"));
        Serial.println(F("╚════════════════════════════════════════════════╝\n"));
        //pulseAtHepsiniTamamla(); //sonra aktif et
        durum = CO_BITTI;
      }
    }
    break;
    
    default:
      break;
  }
}

// ═══════════════════════════════════════════════════════════════
// DURUM BİLGİLERİ
// ═══════════════════════════════════════════════════════════════
bool coAktifMi() {
  return durum != CO_KAPALI && durum != CO_BITTI;
}

bool coTamamlandiMi() {
  return durum == CO_BITTI;
}

void coDurdur() {
  Serial.println(F("[CO] Çift Oynatma Durduruldu!"));
  
  moveToHepsiniDurdur();
  pulseAtHepsiniDurdur();
  oynatmaDurdur();
  
  durum = CO_KAPALI;
}

void coZSifirlamaReset() {
  zSifirlamaTamamlandi = false;
  Serial.println(F("[CO] Z sıfırlama reset edildi."));
}