// OynatmaModulu.h - v5.0 KAYIT BAZLI
// ✅ Kayıt bazlı ölçekleme (pointer ile)

#ifndef OYNATMAMODULU_H
#define OYNATMAMODULU_H

#include <Arduino.h>
#include "stepmotorenkoderiokuma.h"
#include "CiftKayitModulu.h"  // ← CK_Sample için

// ═══════════════════════════════════════════════════════════════
// FONKSİYON TANIMLARI
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Encoder pointer'larını ayarla
 * 
 * KULLANIM (main.cpp setup içinde):
 *   StepMotorEncoder bigEnc(ENC3_A_PIN, ENC3_B_PIN);
 *   StepMotorEncoder zEnc(ENC2_A_PIN, ENC2_B_PIN);
 *   
 *   bigEnc.begin();
 *   zEnc.begin();
 *   oynatmaEncoderSetup(&bigEnc, &zEnc);
 */
void oynatmaEncoderSetup(StepMotorEncoder* bigEncoder, StepMotorEncoder* zEncoder);

/**
 * @brief Dinamik parametre pointer'larını ayarla
 * 
 * KULLANIM (main.cpp setup içinde):
 *   static long bigFreqMin = 20;
 *   static long bigFreqMax = 50;
 *   static long zEncMin = 0;
 *   static long zEncMax = 20000;
 *   
 *   oynatmaParametreSetup(&bigFreqMin, &bigFreqMax, &zEncMin, &zEncMax);
 */
void oynatmaParametreSetup(long* bigFreqMin, long* bigFreqMax, long* zEncMin, long* zEncMax);
void oynatmaRefHizSetup(long* bigFreqRefPtr);
/**
 * @brief ✅ KAYIT BAZLI OYNATMA (pointer ile)
 * 
 * @param kayit Kayıt dizisi (kayit1 veya kayit2)
 * @param ornekSayisi Örnek sayısı
 * 
 * KULLANIM:
 *   // Kayıt1'i oynat
 *   oynatmaBaslatKayit(kayit1, KAYIT_ORNEK_SAYISI);
 * 
 *   // Kayıt2'yi oynat
 *   oynatmaBaslatKayit(kayit2, KAYIT_ORNEK_SAYISI);
 * 
 * ÖZELLİKLER:
 *   - Her kayıt kendi A0 min/max'ını kullanır
 *   - Kayıt1 ve Kayıt2 farklı ölçeklere sahip olabilir
 */
void oynatmaBaslatKayit(const CK_Sample* kayit, uint16_t ornekSayisi);

/**
 * @brief Oynatma arka plan döngüsü (her loop'ta çağrılır)
 */
void oynatmaRun();

/**
 * @brief Oynatma aktif mi?
 */
bool oynatmaAktifMi();

/**
 * @brief Oynatma tamamlandı mı?
 */
bool oynatmaTamamlandiMi();

/**
 * @brief Şu anki segment index
 */
uint16_t oynatmaSegmentIndex();

/**
 * @brief Oynatma acil durdur
 */
void oynatmaDurdur();

#endif // OYNATMAMODULU_H