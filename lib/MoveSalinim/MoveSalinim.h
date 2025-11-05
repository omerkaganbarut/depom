// MoveSalinim.h - X EKSENİ SALINİM MODÜLÜ v2.0
// ═══════════════════════════════════════════════════════════════
// ÖZELLİKLER:
// - Oynatma sırasında X ekseni mevcut konumdan salınım yapar
// - Offset tabanlı (örn: ±2000 encoder birimi)
// - X1 veya X2'den bağımsız çalışır (merkez = şu anki pozisyon)
// - MoveTo modülünü kullanır (ivmeli hareket)
// ═══════════════════════════════════════════════════════════════
//
// ÇALIŞMA MANTIĞI:
// ─────────────────────────────────────────────────────────────
// Oynatma başladığında X motor X1 veya X2'dedir.
// Salınım bu konumdan başlar:
//
// Örnek 1: X = 10000 (X1)
//   Merkez: 10000
//   Salınım: 10000 → 12000 → 8000 → 12000 → 8000...
//            (merkez ± 2000)
//
// Örnek 2: X = 20000 (X2)
//   Merkez: 20000
//   Salınım: 20000 → 22000 → 18000 → 22000 → 18000...
//            (merkez ± 2000)
//
// ═══════════════════════════════════════════════════════════════

#ifndef MOVESALINIM_H
#define MOVESALINIM_H

#include <Arduino.h>
#include "stepmotorenkoderiokuma.h"

// ═══════════════════════════════════════════════════════════════
// FONKSİYON TANIMLARI
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Encoder pointer'ını ayarla (setup'ta bir kez çağır)
 */
void msEncoderSetup(StepMotorEncoder* xEncoder);

/**
 * @brief Salınım başlat (Offset tabanlı)
 * 
 * @param offset Salınım genliği (encoder birimi)
 * @param hiz X motor hızı (Hz)
 * @return true Başarıyla başlatıldı
 * @return false Hata (geçersiz parametre)
 * 
 * KULLANIM:
 *   // Oynatma başladıktan SONRA çağır
 *   msBaslat(2000, 8000);  // ±2000 encoder, 8000Hz
 * 
 * İŞLEYİŞ:
 *   1. Şu anki X pozisyonunu merkez olarak kaydet
 *   2. Merkez + offset → Merkez - offset → döngü
 */
bool msBaslat(long offset, unsigned int hiz);

/**
 * @brief Salınım arka plan döngüsü (her loop'ta çağır!)
 */
void msRun();

/**
 * @brief Salınım aktif mi?
 */
bool msAktifMi();

/**
 * @brief Salınımı durdur
 */
void msDurdur();

/**
 * @brief Salınım durumu bilgisi (debug)
 */
bool msDurumBilgisi(long* merkez, long* offset, long* hedef);

#endif // MOVESALINIM_H