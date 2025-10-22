// KayitModulu.cpp - FINAL VERSION v3 (Otomatik Düzeltmeli)
#include "KayitModulu.h"
#include "Config.h"
#include "PulseAt.h"
#include "stepmotorenkoderiokuma.h"
#include "A0Filtre.h"

// ═══════════════════════════════════════════════════════════════
// DURUM MAKİNESİ
// ═══════════════════════════════════════════════════════════════
enum KayitDurum {
  KAYIT_KAPALI = 0,         // Kayıt başlatılmamış
  KAYIT_BASLANGIC,          // İlk örnek alınıyor
  KAYIT_BASLANGIC_BEKLE,    // 2 saniye bekleniyor
  KAYIT_MOTOR_CALISMA,      // Segment başlatılıyor
  KAYIT_MOTOR_BEKLE,        // Segment tamamlanması bekleniyor
  KAYIT_ORNEK_AL,           // Örnek alınıyor
  KAYIT_TAMAMLANDI          // Tüm örnekler toplandı
};

// ═══════════════════════════════════════════════════════════════
// STATİK DEĞİŞKENLER
// ═══════════════════════════════════════════════════════════════
static KM_Sample samples[KAYIT_ORNEK_SAYISI];  // Kayıt buffer'ı
static uint16_t idx = 0;                        // Mevcut örnek index
static KayitDurum durum = KAYIT_KAPALI;        // Durum makinesi
static unsigned long beklemeBaslangic = 0;     // Bekleme zamanlayıcı
static StepMotorEncoder* bigEnc = nullptr;     // Encoder pointer
static long baslangicEnc = 0;                  // Başlangıç encoder pozisyonu
static int kayitYon = 0;  // Kayıt yönü (0=ileri, 1=geri)
// ═══════════════════════════════════════════════════════════════
// ENCODER SETUP
// ═══════════════════════════════════════════════════════════════
void kayitEncoderSetup(StepMotorEncoder* bigEncoder) {
  bigEnc = bigEncoder;
}

// ═══════════════════════════════════════════════════════════════
// KAYIT BAŞLATMA
// ═══════════════════════════════════════════════════════════════
// ═══════════════════════════════════════════════════════════════
// KAYIT BAŞLATMA - YÖN PARAMETRESİ EKLENDİ
// ═══════════════════════════════════════════════════════════════
void kayitBaslat(int yon) {  // ← PARAMETRE EKLENDİ
  Serial.println(F("\n[KAYIT] Başlatılıyor..."));
  
  idx = 0;
  durum = KAYIT_BASLANGIC;
  
  // Motor enable
  pinMode(ENA1_PIN, OUTPUT);
  pinMode(ENA2_PIN, OUTPUT);
  pinMode(ENA3_PIN, OUTPUT);
  digitalWrite(ENA1_PIN, LOW);
  digitalWrite(ENA2_PIN, LOW);
  digitalWrite(ENA3_PIN, LOW);
  delay(2000);
  
  if (bigEnc == nullptr) {
    Serial.println(F("✗ Encoder hatası!"));
    return;
  }
  
  baslangicEnc = bigEnc->getPosition();
  
  // İlk örneği al
  samples[0].enc = baslangicEnc;
  samples[0].a0 = a0FiltreliOku();
  
  Serial.print(F("  Yön: "));
  Serial.println(yon ? "Geri (16000→0)" : "İleri (0→16000)");
  Serial.print(F("  Başlangıç Enc: "));
  Serial.print(baslangicEnc);
  Serial.print(F(" | İlk A0: "));
  Serial.println(samples[0].a0);
  
  // ═══════════════════════════════════════════════════════════════
  // YÖN DEĞİŞKENİNİ GLOBAL YAPTIK (dosya başında static int kayitYon)
  // ═══════════════════════════════════════════════════════════════
  kayitYon = yon;  // ← Yönü sakla
  
  beklemeBaslangic = millis();
  durum = KAYIT_BASLANGIC_BEKLE;
  
  Serial.println(F("✓ Başlatıldı\n"));
}

// ═══════════════════════════════════════════════════════════════
// KAYIT ARKA PLAN
// ═══════════════════════════════════════════════════════════════
void kayitRun() {
  switch (durum) {
    
    // ───────────────────────────────────────────────────────────
    case KAYIT_KAPALI:
    // ───────────────────────────────────────────────────────────
      // Hiçbir şey yapma
      return;
    
    // ───────────────────────────────────────────────────────────
    case KAYIT_BASLANGIC_BEKLE:
    // ───────────────────────────────────────────────────────────
      // 2 saniye bekle
      if (millis() - beklemeBaslangic >= 2000) {
        durum = KAYIT_MOTOR_CALISMA;
        idx = 1;  // İlk segment için
        
        Serial.print(F("[SEG 0→1]"));
        
        // İlk segment başlat
        useMotor(MOTOR_B);
        pulseAt((unsigned long)KAYIT_ARALIK, kayitYon, (unsigned int)KAYIT_HZ);
        
        durum = KAYIT_MOTOR_BEKLE;
      }
      break;
    
    // ───────────────────────────────────────────────────────────
    case KAYIT_MOTOR_BEKLE:
    // ───────────────────────────────────────────────────────────
      // pulseAt arka planını çalıştır
      useMotor(MOTOR_B);
      pulseAt(0, 0, 0);
      
      // Segment bitti mi?
      if (pulseAtBittiMi(MOTOR_B)) {
        
        // ═════════════════════════════════════════════════════
        // HEDEF KONTROLÜ (Otomatik Düzeltme)
        // ═════════════════════════════════════════════════════
        long mevcutEnc = bigEnc->getPosition();
        
        // Teorik hedef hesapla (kümülatif)
        long teoriHedef = baslangicEnc + (KAYIT_ARALIK * idx);
        
        // Fark hesapla
        long fark = teoriHedef - mevcutEnc;
        
        // Sapma raporu (bilgilendirme)
        if (abs(fark) > 0) {
          Serial.print(F("   ⚠ Sapma: "));
          Serial.print(fark);
          Serial.print(F(" pulse (Hedef: "));
          Serial.print(teoriHedef);
          Serial.print(F(", Mevcut: "));
          Serial.print(mevcutEnc);
          Serial.println(F(")"));
        }
        
        // Örnek almaya geç (düzeltme segmenti YOK!)
        durum = KAYIT_ORNEK_AL;
      }
      break;
    
    // ───────────────────────────────────────────────────────────
    case KAYIT_ORNEK_AL:
    // ───────────────────────────────────────────────────────────
      // Encoder ve A0 oku
      samples[idx].enc = bigEnc->getPosition();
      samples[idx].a0 = a0FiltreliOku();
      
      // Rapor
      Serial.print(F(" → ["));
      Serial.print(idx);
      Serial.print(F("] enc="));
      Serial.print(samples[idx].enc);
      Serial.print(F(", a0="));
      Serial.println(samples[idx].a0);
      
      // Son örnek mi?
      if (idx >= KAYIT_ORNEK_SAYISI - 1) {
        durum = KAYIT_TAMAMLANDI;
        Serial.println(F("\n[KAYIT] Tamamlandı! ✓\n"));
      }
      else {
        // Sonraki segment
        idx++;
        
        Serial.print(F("[SEG "));
        Serial.print(idx - 1);
        Serial.print(F("→"));
        Serial.print(idx);
        Serial.print(F("]"));
        
        // ═════════════════════════════════════════════════════
        // OTOMATİK DÜZELTİLMİŞ SEGMENT
        // ═════════════════════════════════════════════════════
        long mevcutEnc = bigEnc->getPosition();  // Gerçek pozisyon
        long sonrakiTeoriHedef = baslangicEnc + (KAYIT_ARALIK * idx);  // Teorik hedef
        long gerekliFark = sonrakiTeoriHedef - mevcutEnc;  // Gerekli hareket
        
        unsigned long pulseSayisi = (unsigned long)abs(gerekliFark);
        int yon = (gerekliFark > 0) ? 0 : 1;  // Yön belirle
        
        // Debug bilgisi
        if (pulseSayisi != KAYIT_ARALIK) {
          Serial.print(F(" ("));
          Serial.print(pulseSayisi);
          Serial.print(F("p"));
          if (yon != kayitYon) {
            Serial.print(F(", "));
            Serial.print(yon ? F("geri") : F("ileri"));
          }
          Serial.print(F(")"));
        }
        
        // Segment başlat
        useMotor(MOTOR_B);
        pulseAt(pulseSayisi, yon, (unsigned int)KAYIT_HZ);
        
        durum = KAYIT_MOTOR_BEKLE;
      }
      break;
    
    // ───────────────────────────────────────────────────────────
    case KAYIT_TAMAMLANDI:
    // ───────────────────────────────────────────────────────────
      // Hiçbir şey yapma (bitti)
      return;
    
    // ───────────────────────────────────────────────────────────
    default:
    // ───────────────────────────────────────────────────────────
      Serial.println(F("✗ Bilinmeyen durum!"));
      durum = KAYIT_KAPALI;
      break;
  }
}

// ═══════════════════════════════════════════════════════════════
// DURUM FONKSİYONLARI
// ═══════════════════════════════════════════════════════════════

bool kayitAktifMi() {
  return (durum != KAYIT_KAPALI && durum != KAYIT_TAMAMLANDI);
}

bool kayitTamamlandiMi() {
  return (durum == KAYIT_TAMAMLANDI);
}

uint16_t kayitOrnekSayisi() {
  if (durum == KAYIT_TAMAMLANDI) {
    return KAYIT_ORNEK_SAYISI;
  }
  return idx;
}

const KM_Sample* kayitVerileri() {
  return samples;
}

KM_Sample* kayitVerileriDuzenle() {
  return samples;
}

// ═══════════════════════════════════════════════════════════════
// LİSTELEME
// ═══════════════════════════════════════════════════════════════
void kayitListele() {
  if (durum != KAYIT_TAMAMLANDI) {
    Serial.println(F("✗ Kayıt tamamlanmadı!"));
    return;
  }
  
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║            KAYITLI VERİLER                    ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
  
  Serial.println(F(" IDX |  ENCODER  |   A0  |  ENC FARK"));
  Serial.println(F("─────┼───────────┼───────┼──────────"));
  
  for (uint16_t i = 0; i < KAYIT_ORNEK_SAYISI; i++) {
    long encFark = 0;
    if (i > 0) {
      encFark = samples[i].enc - samples[i-1].enc;
    }
    
    char buf[48];
    sprintf(buf, " %3d | %9ld | %5d | %+9ld", 
            i, samples[i].enc, samples[i].a0, encFark);
    Serial.println(buf);
  }
  
  // ─────────────────────────────────────────────────────────────
  // İstatistikler
  // ─────────────────────────────────────────────────────────────
  Serial.println(F("\n─────────────────────────────────────────────────"));
  Serial.println(F("İSTATİSTİKLER:"));
  
  long toplamYol = samples[KAYIT_ORNEK_SAYISI-1].enc - samples[0].enc;
  Serial.print(F("  Toplam Yol    : "));
  Serial.print(toplamYol);
  Serial.println(F(" pulse"));
  
  long teoriYol = KAYIT_ARALIK * (KAYIT_ORNEK_SAYISI - 1);
  Serial.print(F("  Teorik Yol    : "));
  Serial.print(teoriYol);
  Serial.println(F(" pulse"));
  
  long toplamSapma = toplamYol - teoriYol;
  Serial.print(F("  Toplam Sapma  : "));
  Serial.print(toplamSapma);
  Serial.print(F(" pulse ("));
  Serial.print((float)toplamSapma * 100.0 / teoriYol, 2);
  Serial.println(F("%)"));
  
  // A0 aralığı
  uint16_t a0Min = samples[0].a0;
  uint16_t a0Max = samples[0].a0;
  for (uint16_t i = 1; i < KAYIT_ORNEK_SAYISI; i++) {
    if (samples[i].a0 < a0Min) a0Min = samples[i].a0;
    if (samples[i].a0 > a0Max) a0Max = samples[i].a0;
  }
  
  Serial.print(F("  A0 Min        : "));
  Serial.println(a0Min);
  Serial.print(F("  A0 Max        : "));
  Serial.println(a0Max);
  Serial.print(F("  A0 Aralık     : "));
  Serial.println(a0Max - a0Min);
  
  Serial.println();
}

// ═══════════════════════════════════════════════════════════════
// DURDURMA
// ═══════════════════════════════════════════════════════════════
void kayitDurdur() {
  Serial.println(F("\n[KAYIT] Durduruldu!"));
  
  // Motor acil durdur
  pulseAtDurdur(MOTOR_B);
  
  // Durumu sıfırla
  durum = KAYIT_KAPALI;
  
  // Rapor
  Serial.print(F("   Toplanan örnek: "));
  Serial.println(idx);
  Serial.println();
}