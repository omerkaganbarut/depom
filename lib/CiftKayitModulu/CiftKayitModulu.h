// CiftKayitModulu.h - İKİ FARKLI X POZİSYONUNDA KAYIT ALMA
#ifndef CIFTKAYITMODULU_H
#define CIFTKAYITMODULU_H

#include <Arduino.h>
#include "stepmotorenkoderiokuma.h"
#include "Config.h"

// ═══════════════════════════════════════════════════════════════
// KAYIT VERİ YAPISI
// ═══════════════════════════════════════════════════════════════
struct CK_Sample {
  long enc;      // BIG encoder pozisyonu
  uint16_t a0;   // A0 sensör değeri
};

// ═══════════════════════════════════════════════════════════════
// ✅ EXTERN KAYIT ARRAYLERI (CiftOynatmaModulu erişebilir)
// ═══════════════════════════════════════════════════════════════
extern CK_Sample kayit1[KAYIT_ORNEK_SAYISI];
extern CK_Sample kayit2[KAYIT_ORNEK_SAYISI];

// ═══════════════════════════════════════════════════════════════
// FONKSİYON TANIMLARI
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Encoder'ları ayarla (setup'ta bir kez çağır)
 */
void ckEncoderSetup(StepMotorEncoder* bigEncoder, StepMotorEncoder* xEncoder);

/**
 * @brief Çift kayıt işlemini başlat
 * 
 * @param x1Enc X1 pozisyonu (encoder değeri)
 * @param x2Enc X2 pozisyonu (encoder değeri)
 * @param kayit1Yon Kayıt1 yönü (0=ileri 0→16000, 1=geri 16000→0)
 * @param kayit2Yon Kayıt2 yönü
 * 
 * İŞLEYİŞ:
 *   1. X1'e git → Y/N → Kayıt1 al
 *   2. X2'ye git → Y/N → Kayıt2 al
 *   3. Global A0_min/max hesapla
 */
void ckBaslat(long x1Enc, long x2Enc, int kayit1Yon, int kayit2Yon);

/**
 * @brief Çift kayıt arka plan döngüsü (her loop'ta çağır!)
 */
void ckRun();

/**
 * @brief Çift kayıt aktif mi?
 */
bool ckAktifMi();

/**
 * @brief Çift kayıt tamamlandı mı?
 */
bool ckTamamlandiMi();

/**
 * @brief Kayıt1 verilerini listele (Serial'e yazdır)
 */
void ckKayit1Listele();

/**
 * @brief Kayıt2 verilerini listele (Serial'e yazdır)
 */
void ckKayit2Listele();

/**
 * @brief Çift kayıt işlemini durdur (acil durdurma)
 */
void ckDurdur();

// CiftKayitModulu.h dosyasına ekle:

// Global A0 aralığı (extern - başka modüller kullanabilir)
extern uint16_t globalA0Min;
extern uint16_t globalA0Max;

#endif // CIFTKAYITMODULU_H