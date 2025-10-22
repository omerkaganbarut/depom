// OynatmaModulu.h - v2.4
// ✅ Z encoder desteği
// ✅ Dinamik parametre sistemi

#ifndef OYNATMAMODULU_H
#define OYNATMAMODULU_H

#include <Arduino.h>
#include "stepmotorenkoderiokuma.h"

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

/**
 * @brief Oynatma işlemini başlatır (gerçek başlatma)
 * 
 * NOT: Ana menüden "O" komutuyla DOĞRUDAN çağrılmaz!
 *      Önce parametre onayı alınır, sonra bu fonksiyon çağrılır.
 */
void oynatmaBaslatGercek();

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