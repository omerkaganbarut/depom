// MoveTo.h - İVMELİ HAREKET MODÜLÜ v4 - SALINİM MODU EKLENDİ
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
 */
void moveToSetup(StepMotorEncoder* encZ, 
                 StepMotorEncoder* encX, 
                 StepMotorEncoder* encB);

/**
 * @brief İvmeli hareket başlat (Motor bazlı rampa + Salınım modu)
 * 
 * @param motorIndex Motor (MOTOR_Z=0, MOTOR_X=1, MOTOR_B=2)
 * @param hedefEnc Hedef encoder pozisyonu
 * @param maxHz Maximum hız (Hz)
 * @param salinimModu true = Salınım modu (hızlı rampa)
 *                    false = Normal mod (default)
 * 
 * KULLANIM:
 *   moveTo(MOTOR_X, 10000, 5000);        // Normal: 6000p rampa
 *   moveTo(MOTOR_X, 10000, 8000, true);  // Salınım: 2000p rampa
 */
bool moveTo(uint8_t motorIndex, long hedefEnc, unsigned int maxHz, bool salinimModu = false);

bool moveToAktifMi(uint8_t motorIndex);
void moveToRun();
bool moveToBittiMi(uint8_t motorIndex);
void moveToDurdur(uint8_t motorIndex);
void moveToHepsiniDurdur();

#endif // MOVETO_H