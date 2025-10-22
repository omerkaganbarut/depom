// CiftKayitModulu.h - İKİ FARKLI X POZİSYONUNDA KAYIT ALMA
#ifndef CIFTKAYITMODULU_H
#define CIFTKAYITMODULU_H

#include <Arduino.h>
#include "stepmotorenkoderiokuma.h"

// ═══════════════════════════════════════════════════════════════
// KAYIT VERİ YAPISI
// ═══════════════════════════════════════════════════════════════
struct CK_Sample {
  long enc;      // BIG encoder pozisyonu
  uint16_t a0;   // A0 sensör değeri
};

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
 * @brief Global A0 minimum değerini al
 */
uint16_t ckGlobalA0Min();

/**
 * @brief Global A0 maximum değerini al
 */
uint16_t ckGlobalA0Max();

/**
 * @brief Kayıt1 verilerine eriş (read-only)
 */
const CK_Sample* ckKayit1Verileri();

/**
 * @brief Kayıt2 verilerine eriş (read-only)
 */
const CK_Sample* ckKayit2Verileri();

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

#endif // CIFTKAYITMODULU_H