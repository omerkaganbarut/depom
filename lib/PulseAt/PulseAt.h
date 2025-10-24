// PulseAt.h - STEP MOTOR PARALEL SÜRÜŞ MODÜLÜ v3
#ifndef PULSEAT_H
#define PULSEAT_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════
// FONKSİYON TANIMLARI
// ═══════════════════════════════════════════════════════════════

void useMotor(uint8_t motorIndex);
void pulseAt(unsigned long toplamPulse, int yon, unsigned int hertz);
bool pulseAtAktifMi(uint8_t motorIndex);
bool pulseAtBittiMi(uint8_t motorIndex);

// ═══════════════════════════════════════════════════════════════
// ÜÇ TİP DURDURMA FONKSİYONU
// ═══════════════════════════════════════════════════════════════

/**
 * @brief 1️⃣ SEGMENT GEÇİŞİ DURDURMA (ENA LOW kalsın)
 * 
 * NE YAPAR:
 *   - Sadece pulse gönderimini durdurur
 *   - Motor fiziksel olarak AKTİF kalır (ENA LOW)
 *   - Motor pozisyonunu koruya devam eder
 * 
 * NE ZAMAN KULLAN:
 *   ✅ Segment geçişlerinde (Kayıt/Oynatma)
 *   ✅ Başlangıç konumunda beklerken (Y/N onayı)
 *   ✅ Pozisyon korunmalı durumlarda
 * 
 * KULLANIM:
 *   // Kayıt segment geçişi
 *   if (pulseAtBittiMi(MOTOR_B)) {
 *       pulseAtDurdur(MOTOR_B);  // ← Motor pozisyonu korunsun
 *       // Sonraki segment hazırla...
 *   }
 * 
 *   // Başlangıç konumu bekleme
 *   moveTo(MOTOR_X, x1Pos, 5000);
 *   while (moveToAktifMi(MOTOR_X)) { moveToRun(); }
 *   pulseAtDurdur(MOTOR_X);  // ← Motor sabit dursun
 *   Serial.println("Y/N?");
 */
void pulseAtDurdur(uint8_t motorIndex);

/**
 * @brief 2️⃣ İŞLEM TAMAMLAMA DURDURMA (ENA HIGH - enerji kes)
 * 
 * NE YAPAR:
 *   - Pulse gönderimini durdurur
 *   - Motor fiziksel olarak DEVRE DIŞI bırakılır (ENA HIGH)
 *   - Enerji tasarrufu sağlar
 * 
 * NE ZAMAN KULLAN:
 *   ✅ Kayıt/Oynatma tamamen bittiğinde
 *   ✅ MoveTo tamamlandığında (uzun süreli)
 *   ✅ Kullanıcı başka işlerle uğraşırken
 * 
 * KULLANIM:
 *   // Kayıt tamamlandı
 *   if (kayitTamamlandiMi()) {
 *       pulseAtTamamla(MOTOR_B);  // ← Enerji kes
 *       Serial.println("Kayıt tamam!");
 *   }
 * 
 *   // Oynatma bitti
 *   if (oynatmaTamamlandiMi()) {
 *       pulseAtTamamla(MOTOR_B);  // ← Motorları dinlendir
 *       pulseAtTamamla(MOTOR_Z);
 *       Serial.println("Oynatma tamam!");
 *   }
 */
void pulseAtTamamla(uint8_t motorIndex);

/**
 * @brief 3️⃣ ACİL DURDURMA (ENA HIGH - hemen kes!)
 * 
 * NE YAPAR:
 *   - Pulse gönderimini durdurur
 *   - Motor fiziksel olarak HEMEN DURDURULUR (ENA HIGH)
 *   - Acil durum için kullanılır
 * 
 * NE ZAMAN KULLAN:
 *   ✅ Acil durdurma butonu ('S' komutu)
 *   ✅ Hata durumlarında
 *   ✅ Kullanıcı müdahalesi gerektiğinde
 * 
 * KULLANIM:
 *   // Acil durdurma komutu
 *   if (cmd[0] == 'S') {
 *       pulseAtAcilDurdur(MOTOR_Z);
 *       pulseAtAcilDurdur(MOTOR_X);
 *       pulseAtAcilDurdur(MOTOR_B);
 *       Serial.println("ACİL DURDURMA!");
 *   }
 */
void pulseAtAcilDurdur(uint8_t motorIndex);

/**
 * @brief Tüm motorları ACİL DURDUR
 */
void pulseAtHepsiniDurdur();

/**
 * @brief Tüm motorları TAMAMLA (enerji kes)
 */
void pulseAtHepsiniTamamla();

// ═══════════════════════════════════════════════════════════════
// HANGİSİNİ KULLANMALISIN?
// ═══════════════════════════════════════════════════════════════
/*

┌─────────────────────────────────────────────────────────────┐
│ DURUM                           │ FONKSİYON                  │
├─────────────────────────────────┼────────────────────────────┤
│ Segment geçişi                  │ pulseAtDurdur()            │
│ Başlangıç konumu bekleme        │ pulseAtDurdur()            │
│ Pozisyon korunmalı              │ pulseAtDurdur()            │
├─────────────────────────────────┼────────────────────────────┤
│ Kayıt/Oynatma tamamlandı        │ pulseAtTamamla()           │
│ MoveTo bitti (uzun süreli)      │ pulseAtTamamla()           │
│ Kullanıcı başka iş yapıyor      │ pulseAtTamamla()           │
├─────────────────────────────────┼────────────────────────────┤
│ Acil durdurma butonu            │ pulseAtAcilDurdur()        │
│ Hata durumu                     │ pulseAtAcilDurdur()        │
│ 'S' komutu                      │ pulseAtHepsiniDurdur()     │
└─────────────────────────────────┴────────────────────────────┘

ÖZET:
  pulseAtDurdur()       → ENA LOW  → Motor pozisyon tutar
  pulseAtTamamla()      → ENA HIGH → Enerji tasarrufu
  pulseAtAcilDurdur()   → ENA HIGH → Acil durdurma

*/

#endif // PULSEAT_H