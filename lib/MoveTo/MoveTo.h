// MoveTo.h - İVMELİ HAREKET MODÜLÜ v2 FINAL
#ifndef MOVETO_H
#define MOVETO_H

#include <Arduino.h>
#include "stepmotorenkoderiokuma.h"
#include "Config.h"

// ═══════════════════════════════════════════════════════════════
// İVME PARAMETRELERİ
// ═══════════════════════════════════════════════════════════════
#define ACCEL_RAMP_PULSE  200   // İvme/fren mesafesi (pulse)
#define MIN_SPEED_HZ      50    // Minimum hız (kalkış/duruş)

// ═══════════════════════════════════════════════════════════════
// EĞİTİM: İvmeli Hareket Sistemi
// ═══════════════════════════════════════════════════════════════
/*
┌──────────────────────────────────────────────────────────────┐
│  NEDEN İVME GEREKLİ?                                         │
└──────────────────────────────────────────────────────────────┘

ANI HIZ DEĞİŞİMİ (İvmesiz):
  Hız
   │
100Hz┤  ████████████  ← Motor şoka girer
     │  █          █
     │  █          █
  0  ┤──█──────────█──
     └──────────────────→ Zaman
       KAYMA!   KAYMA!

YUMUŞAK GEÇİŞ (İvmeli):
  Hız
   │
100Hz┤     ╱────────╲   ← Yumuşak kalkış/duruş
     │   ╱            ╲
  50Hz┤ ╱              ╲
     │╱                  ╲
  0  ┤────────────────────
     └──────────────────────→ Zaman
     KALKIŞ  SEYIR  FREN

┌──────────────────────────────────────────────────────────────┐
│  TRAPEZOID PROFİL (3 Faz)                                    │
└──────────────────────────────────────────────────────────────┘

FAZ 1: HIZLANMA (İlk 100 pulse)
  10Hz → 100Hz (lineer artış)
  Her pulse'da hız: 10 + (100-10) × (tamamlanan/100)

FAZ 2: SABİT HIZ (Orta kısım)
  100Hz sabit

FAZ 3: YAVAŞLAMA (Son 100 pulse)
  100Hz → 10Hz (lineer azalış)
  Her pulse'da hız: 10 + (100-10) × (kalan/100)

┌──────────────────────────────────────────────────────────────┐
│  ÖNEMLI: TOPLAM PULSE ASLA DEĞİŞMEZ!                        │
└──────────────────────────────────────────────────────────────┘

✅ DOĞRU:
  Hedef: 5000, Mevcut: 2000
  Toplam pulse: |5000-2000| = 3000
  İvme sadece hız profilini değiştirir
  Sonuçta 3000 pulse atılır ✓

❌ YANLIŞ:
  Toplam pulse: 3000
  İvme için ekstra: +200
  Toplam: 3200 pulse → Hedefi aşar! ✗

┌──────────────────────────────────────────────────────────────┐
│  KULLANIM                                                     │
└──────────────────────────────────────────────────────────────┘

SETUP:
  StepMotorEncoder bigEnc(ENC3_A_PIN, ENC3_B_PIN);
  StepMotorEncoder xEnc(ENC1_A_PIN, ENC1_B_PIN);
  StepMotorEncoder zEnc(ENC2_A_PIN, ENC2_B_PIN);
  
  bigEnc.begin();
  xEnc.begin();
  zEnc.begin();
  
  moveToSetup(&zEnc, &xEnc, &bigEnc);

LOOP:
  // Hareket başlat
  moveTo(MOTOR_B, 5000, 100);  // Encoder'ı 5000'e, max 100Hz
  
  // Her loop'ta arka plan çalıştır
  moveToRun();
  
  // Bitti mi kontrol et
  if (moveToBittiMi(MOTOR_B)) {
    Serial.println("Hedefe ulaşıldı!");
  }

ÖRNEK SENARYOLAR:
  1. Kısa mesafe (50 pulse):
     - Sadece yavaşlama fazı (FAZ_YAVASLAMA)
     - 10Hz → hedefe ulaş
  
  2. Orta mesafe (150 pulse):
     - Hızlanma (50p) + Yavaşlama (100p)
     - Sabit hız fazı yok
  
  3. Uzun mesafe (1000 pulse):
     - Hızlanma (100p) + Sabit (800p) + Yavaşlama (100p)
     - Tam trapezoid profil
*/

// ═══════════════════════════════════════════════════════════════
// FONKSİYON TANIMLARI
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Encoder pointer'larını kaydet
 * 
 * @param encZ Z motor encoder'ı
 * @param encX X motor encoder'ı
 * @param encB BIG motor encoder'ı
 */
void moveToSetup(StepMotorEncoder* encZ, 
                 StepMotorEncoder* encX, 
                 StepMotorEncoder* encB);

/**
 * @brief İvmeli hareket başlat
 * 
 * @param motorIndex Motor (MOTOR_Z=0, MOTOR_X=1, MOTOR_B=2)
 * @param hedefEnc Hedef encoder pozisyonu
 * @param maxHz Maximum hız (Hz)
 * @return true Başarılı
 * @return false Başarısız (motor aktif, geçersiz parametre, vb)
 * 
 * ÖRNEKLER:
 *   moveTo(MOTOR_B, 5000, 100);   // BIG → 5000, max 100Hz
 *   moveTo(MOTOR_Z, -1000, 50);   // Z → -1000, max 50Hz
 *   moveTo(MOTOR_X, 0, 80);       // X → home (0), max 80Hz
 * 
 * İVME PROFİLİ:
 *   - Mesafe < 100p: Sadece yavaşlama
 *   - Mesafe 100-200p: Hızlanma + yavaşlama
 *   - Mesafe > 200p: Tam trapezoid profil
 */
bool moveTo(uint8_t motorIndex, long hedefEnc, unsigned int maxHz);

/**
 * @brief MoveTo arka plan döngüsü (her loop'ta çağır!)
 * 
 * NE YAPAR:
 *   - Aktif motorların pulseAt() arka planını çalıştırır
 *   - Segment tamamlandığında sonraki segmenti başlatır
 *   - Faz geçişlerini yönetir (hızlanma→sabit→yavaşlama)
 *   - Hedefe ulaşınca motoru durdurur
 * 
 * KULLANIM:
 *   void loop() {
 *     moveToRun();  // ← Mutlaka her loop'ta çağır!
 *     // ... diğer kodlar ...
 *   }
 */
void moveToRun();

/**
 * @brief Motor hedefe ulaştı mı? (edge detection)
 * 
 * @param motorIndex Motor numarası
 * @return true Motor az önce hedefe ulaştı (bu çağrıda)
 * @return false Motor bitmedi veya zaten kontrol edildi
 * 
 * ÖNEMLİ: Edge detection! İlk çağrıda true, sonrakilerde false
 * 
 * ÖRNEK:
 *   if (moveToBittiMi(MOTOR_B)) {
 *     Serial.println("Motor durdu!");  // Sadece 1 kez yazdırır
 *   }
 */
bool moveToBittiMi(uint8_t motorIndex);

/**
 * @brief Motor aktif mi?
 * 
 * @return true Motor hareket ediyor
 * @return false Motor durmuş
 */
bool moveToAktifMi(uint8_t motorIndex);

/**
 * @brief Hedefe kalan pulse sayısı
 * 
 * @return long Kalan pulse (0 = hedefe ulaştı)
 */
long moveToKalan(uint8_t motorIndex);

/**
 * @brief Motoru acil durdur
 * 
 * @param motorIndex Motor numarası
 * 
 * NE YAPAR:
 *   - pulseAt'ı durdurur
 *   - Motor durumunu sıfırlar
 *   - Kalan yol iptal edilir
 */
void moveToDurdur(uint8_t motorIndex);

/**
 * @brief Tüm motorları acil durdur
 */
void moveToHepsiniDurdur();

#endif // MOVETO_H