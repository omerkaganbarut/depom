// CiftOynatmaModulu.h - İKİ KAYDI SIRAYLA OYNATMA + GÜVENLİKLİ GEÇİŞ
#ifndef CIFTOYNATMAMODULU_H
#define CIFTOYNATMAMODULU_H

#include <Arduino.h>
#include "stepmotorenkoderiokuma.h"

// ═══════════════════════════════════════════════════════════════
// FONKSİYON TANIMLARI
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Encoder'ları ayarla (setup'ta bir kez çağır)
 */
void coEncoderSetup(StepMotorEncoder* bigEncoder, 
                    StepMotorEncoder* xEncoder,
                    StepMotorEncoder* zEncoder);

/**
 * @brief Parametre pointer'larını ayarla (setup'ta bir kez çağır)
 */
void coParametreSetup(long* bigFreqMin, long* bigFreqMax, 
                      long* zEncMin, long* zEncMax);

/**
 * @brief Çift oynatma işlemini başlat
 * 
 * @param x1Enc X1 pozisyonu (Kayıt1 için)
 * @param x2Enc X2 pozisyonu (Kayıt2 için)
 * 
 * İŞLEYİŞ:
 *   1. BIG & X → A0_min konumuna
 *   2. Manuel torch sürme → Y/N → Z=0
 *   3. Kayıt1 Geçişi:
 *      - Z yukarı (A0_max)
 *      - BIG & X → Kayıt1[0]
 *      - Z aşağı (Kayıt1[0].a0)
 *   4. Kayıt1 oynat
 *   5. Kayıt2 Geçişi: (aynı mekanizma)
 *   6. Kayıt2 oynat
 */
void coBaslat(long x1Enc, long x2Enc);

/**
 * @brief Çift oynatma arka plan döngüsü (her loop'ta çağır!)
 */
void coRun();
/**
 * @brief Çift oynatma işlemini durdur (acil durdurma)
 */
void coDurdur();

/**
 * @brief Z sıfırlama bayrağını sıfırla (Yeni kayıt alındığında çağrılır)
 * 
 * AÇIKLAMA: CiftKayitModulu'nden yeni kayıt alındığında bu fonksiyon
 *           çağrılır. Bir sonraki CO komutunda Z sıfırlama aşaması
 *           tekrar çalışır.
 */
void coZSifirlamaReset();



#endif // CIFTOYNATMAMODULU_H