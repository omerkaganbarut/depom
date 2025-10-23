// MoveTo.h - İVMELİ HAREKET MODÜLÜ v3 - MOTOR BAZLI RAMPA + KESİN HEDEF
#ifndef MOVETO_H
#define MOVETO_H

#include <Arduino.h>
#include "stepmotorenkoderiokuma.h"
#include "Config.h"

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
 * @brief İvmeli hareket başlat (Motor bazlı rampa)
 * 
 * @param motorIndex Motor (MOTOR_Z=0, MOTOR_X=1, MOTOR_B=2)
 * @param hedefEnc Hedef encoder pozisyonu
 * @param maxHz Maximum hız (Hz)
 * @return true Başarılı
 * @return false Başarısız
 * 
 * YENİ ÖZELLİKLER:
 *   - Motor bazlı rampa (Z:4000p, X:3000p, BIG:200p)
 *   - Kesin hedef (tolerans yok, matematik olarak kesin)
 *   - Encoder'dan bağımsız pulse hesabı
 */
bool moveTo(uint8_t motorIndex, long hedefEnc, unsigned int maxHz);

/**
 * @brief MoveTo arka plan döngüsü (her loop'ta çağır!)
 */
void moveToRun();

/**
 * @brief Motor hedefe ulaştı mı? (edge detection)
 */
bool moveToBittiMi(uint8_t motorIndex);

/**
 * @brief Motor aktif mi?
 */
bool moveToAktifMi(uint8_t motorIndex);

/**
 * @brief Hedefe kalan pulse sayısı
 */
long moveToKalan(uint8_t motorIndex);

/**
 * @brief Motoru acil durdur
 */
void moveToDurdur(uint8_t motorIndex);

/**
 * @brief Tüm motorları acil durdur
 */
void moveToHepsiniDurdur();

#endif // MOVETO_H