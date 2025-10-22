// PulseAt.h - STEP MOTOR PARALEL SÜRÜŞ MODÜLÜ
#ifndef PULSEAT_H
#define PULSEAT_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════
// MOTOR İNDEX TANIMLARI (Config.h'den gelir)
// ═══════════════════════════════════════════════════════════════
// MOTOR_Z = 0  (1. motor - Z ekseni)
// MOTOR_X = 1  (2. motor - X ekseni)
// MOTOR_B = 2  (3. motor - BIG motor)

// ═══════════════════════════════════════════════════════════════
// FONKSİYON TANIMLARI
// ═══════════════════════════════════════════════════════════════

/**
 * @brief Aktif motoru seçer
 * 
 * @param motorIndex Motor numarası (MOTOR_Z=0, MOTOR_X=1, MOTOR_B=2)
 * 
 * NE YAPAR:
 *   - Sonraki pulseAt() çağrılarının hangi motor için olduğunu belirler
 *   - Bu fonksiyon çağrıldıktan sonra pulseAt() ilgili motoru kontrol eder
 * 
 * KULLANIM:
 *   useMotor(MOTOR_B);          // BIG motoru seç
 *   pulseAt(1000, 0, 50);       // BIG motor için iş başlat
 *   
 *   useMotor(MOTOR_X);          // X motoru seç
 *   pulseAt(500, 1, 100);       // X motor için iş başlat
 * 
 * NOT: Her motor için ayrı useMotor() çağrısı gerekir!
 */
void useMotor(uint8_t motorIndex);

/**
 * @brief Motor için pulse gönderimi başlatır veya arka planını çalıştırır
 * 
 * @param toplamPulse Gönderilecek toplam pulse sayısı (0 = arka plan modu)
 * @param yon Yön (0=ileri, 1=geri)
 * @param hertz Frekans (Hz cinsinden)
 * 
 * İKİ KULLANIM ŞEKLİ:
 * 
 * [1] YENİ İŞ BAŞLATMA (toplamPulse > 0):
 *     useMotor(MOTOR_B);
 *     pulseAt(1000, 0, 50);  // 1000 pulse, ileri, 50Hz
 *     
 *     NE YAPAR:
 *       - Seçili motorun durumunu ayarlar (aktif=true, hedef=1000, vb)
 *       - DIR pinini ayarlar (yön)
 *       - Enable pinini aktif eder (LOW)
 *       - İlk pulse zamanını kaydeder
 * 
 * [2] ARKA PLAN İŞLEYİCİSİ (toplamPulse = 0):
 *     useMotor(MOTOR_B);
 *     pulseAt(0, 0, 0);      // Arka plan modu
 *     
 *     NE YAPAR:
 *       - Motor aktif değilse hiçbir şey yapmaz (hızlı return)
 *       - Zaman kontrolü yapar (micros() ile)
 *       - Sıra geldiyse bir pulse üretir (STEP pin HIGH→LOW)
 *       - Hedefe ulaşıldıysa motoru durdurur (aktif=false, bittiEdge=true)
 * 
 * KULLANIM ÖRNEĞİ:
 *   // İş başlat
 *   useMotor(MOTOR_B);
 *   pulseAt(2000, 0, 50);
 *   
 *   // Loop içinde arka planı çalıştır
 *   while (pulseAtAktifMi(MOTOR_B)) {
 *       useMotor(MOTOR_B);
 *       pulseAt(0, 0, 0);  // ← ARKA PLAN İŞLEYİCİSİ
 *   }
 * 
 * PARALEL KULLANIM:
 *   // Üç motoru da başlat
 *   useMotor(MOTOR_Z); pulseAt(500, 0, 30);
 *   useMotor(MOTOR_X); pulseAt(1000, 1, 100);
 *   useMotor(MOTOR_B); pulseAt(2000, 0, 50);
 *   
 *   // Loop içinde hepsini işle
 *   loop() {
 *       useMotor(MOTOR_Z); pulseAt(0, 0, 0);
 *       useMotor(MOTOR_X); pulseAt(0, 0, 0);
 *       useMotor(MOTOR_B); pulseAt(0, 0, 0);
 *   }
 */
void pulseAt(unsigned long toplamPulse, int yon, unsigned int hertz);

/**
 * @brief Motorun aktif olup olmadığını kontrol eder
 * 
 * @param motorIndex Motor numarası (MOTOR_Z=0, MOTOR_X=1, MOTOR_B=2)
 * @return true Motor çalışıyor (pulse gönderimi devam ediyor)
 * @return false Motor durmuş veya hiç başlatılmamış
 * 
 * KULLANIM:
 *   if (pulseAtAktifMi(MOTOR_B)) {
 *       Serial.println("BIG motor çalışıyor...");
 *       
 *       // Arka plan işleyicisini çağır
 *       useMotor(MOTOR_B);
 *       pulseAt(0, 0, 0);
 *   }
 * 
 * WHILE İLE KULLANIM (blocking):
 *   while (pulseAtAktifMi(MOTOR_B)) {
 *       useMotor(MOTOR_B);
 *       pulseAt(0, 0, 0);
 *   }
 *   Serial.println("Motor durdu!");
 * 
 * IF İLE KULLANIM (non-blocking):
 *   if (pulseAtAktifMi(MOTOR_B)) {
 *       useMotor(MOTOR_B);
 *       pulseAt(0, 0, 0);
 *   } else {
 *       Serial.println("Motor durdu!");
 *   }
 */
bool pulseAtAktifMi(uint8_t motorIndex);

/**
 * @brief Motorun işini bitirip bitirmediğini kontrol eder (edge detection)
 * 
 * @param motorIndex Motor numarası (MOTOR_Z=0, MOTOR_X=1, MOTOR_B=2)
 * @return true Motor az önce bitti (bu çağrıda)
 * @return false Motor bitmedi veya zaten kontrol edildi
 * 
 * NE YAPAR:
 *   - Motor hedefe ulaştığında bir bayrak kaldırılır (bittiEdge = true)
 *   - Bu fonksiyon bayrağı okur ve indirir (bittiEdge = false)
 *   - Böylece "bitti" olayı sadece bir kez yakalanır
 * 
 * KULLANIM:
 *   if (pulseAtBittiMi(MOTOR_B)) {
 *       Serial.println("BIG motor işini tamamladı!");
 *       // Bir sonraki işe geç...
 *   }
 * 
 * DURUM MAKİNESİ İLE KULLANIM:
 *   switch (durum) {
 *       case MOTOR_CALISIYOR:
 *           useMotor(MOTOR_B);
 *           pulseAt(0, 0, 0);
 *           
 *           if (pulseAtBittiMi(MOTOR_B)) {
 *               durum = MOTOR_DURDU;
 *           }
 *           break;
 *       
 *       case MOTOR_DURDU:
 *           Serial.println("Motor durdu, yeni iş başlat");
 *           // ...
 *           break;
 *   }
 * 
 * ÖNEMLİ: Bu fonksiyon "edge detector" gibi çalışır.
 *         İlk çağrıda true döner, sonraki çağrılarda false döner.
 *         Bu sayede "bitti" olayını kaçırmazsın!
 */
bool pulseAtBittiMi(uint8_t motorIndex);

/**
 * @brief Belirtilen motoru acil durdurur
 * 
 * @param motorIndex Motor numarası (MOTOR_Z=0, MOTOR_X=1, MOTOR_B=2)
 * 
 * NE YAPAR:
 *   - Motor durumunu sıfırlar (aktif=false)
 *   - Sayaçları temizler (sayac=0, hedef=0)
 *   - Bitiş bayrağını sıfırlar (bittiEdge=false)
 *   - Motor yarıda kesilir (kalan pulse'lar iptal olur)
 * 
 * KULLANIM:
 *   if (emergencyStop) {
 *       pulseAtDurdur(MOTOR_B);
 *       Serial.println("BIG motor durduruldu!");
 *   }
 * 
 * NOT: Enable pinini kapatmaz, sadece pulse gönderimini durdurur
 */
void pulseAtDurdur(uint8_t motorIndex);

/**
 * @brief Tüm motorları acil durdurur
 * 
 * NE YAPAR:
 *   - 3 motorun da durumunu sıfırlar
 *   - pulseAtDurdur()'u her motor için çağırır
 * 
 * KULLANIM:
 *   if (digitalRead(EMERGENCY_STOP_PIN) == LOW) {
 *       pulseAtHepsiniDurdur();
 *       Serial.println("ACİL DURDURMA - TÜM MOTORLAR DURDURULDU!");
 *   }
 */
void pulseAtHepsiniDurdur();

// ═══════════════════════════════════════════════════════════════
// KULLANIM ÖRNEKLERİ
// ═══════════════════════════════════════════════════════════════
/*

╔═══════════════════════════════════════════════════════════════╗
║                    ÖRNEK 1: TEK MOTOR                          ║
╚═══════════════════════════════════════════════════════════════╝

void loop() {
  static bool baslatildi = false;
  
  // İş başlat (bir kez)
  if (!baslatildi) {
    useMotor(MOTOR_B);
    pulseAt(2000, 0, 50);  // 2000 pulse, ileri, 50Hz
    baslatildi = true;
  }
  
  // Arka planı çalıştır
  if (pulseAtAktifMi(MOTOR_B)) {
    useMotor(MOTOR_B);
    pulseAt(0, 0, 0);  // ← Her loop'ta çağır
  }
  
  // Bittiğinde bilgi ver
  if (pulseAtBittiMi(MOTOR_B)) {
    Serial.println("Motor tamamlandı!");
    baslatildi = false;  // Tekrar başlatılabilir
  }
}

╔═══════════════════════════════════════════════════════════════╗
║                 ÖRNEK 2: PARALEL 3 MOTOR                       ║
╚═══════════════════════════════════════════════════════════════╝

void loop() {
  static bool baslatildi = false;
  
  // Üç motoru da başlat (bir kez)
  if (!baslatildi && Serial.read() == 'P') {
    useMotor(MOTOR_Z); pulseAt(500, 0, 30);
    useMotor(MOTOR_X); pulseAt(1000, 1, 100);
    useMotor(MOTOR_B); pulseAt(2000, 0, 50);
    baslatildi = true;
  }
  
  // Her motorun arka planını çalıştır
  if (pulseAtAktifMi(MOTOR_Z)) {
    useMotor(MOTOR_Z);
    pulseAt(0, 0, 0);
  }
  
  if (pulseAtAktifMi(MOTOR_X)) {
    useMotor(MOTOR_X);
    pulseAt(0, 0, 0);
  }
  
  if (pulseAtAktifMi(MOTOR_B)) {
    useMotor(MOTOR_B);
    pulseAt(0, 0, 0);
  }
  
  // Bitenleri kontrol et
  if (pulseAtBittiMi(MOTOR_Z)) {
    Serial.println("Motor Z bitti!");
  }
  if (pulseAtBittiMi(MOTOR_X)) {
    Serial.println("Motor X bitti!");
  }
  if (pulseAtBittiMi(MOTOR_B)) {
    Serial.println("Motor B bitti!");
  }
  
  // Hepsi bitti mi?
  if (baslatildi && !pulseAtAktifMi(MOTOR_Z) 
                 && !pulseAtAktifMi(MOTOR_X) 
                 && !pulseAtAktifMi(MOTOR_B)) {
    Serial.println("TÜM MOTORLAR TAMAMLANDI!");
    baslatildi = false;
  }
}

╔═══════════════════════════════════════════════════════════════╗
║              ÖRNEK 3: DURUM MAKİNESİ İLE                       ║
╚═══════════════════════════════════════════════════════════════╝

enum Durum { BEKLIYOR, MOTOR_CALISIYOR, MOTOR_DURDU };
Durum durum = BEKLIYOR;

void loop() {
  switch (durum) {
    case BEKLIYOR:
      if (Serial.read() == 'G') {
        useMotor(MOTOR_B);
        pulseAt(1000, 0, 50);
        durum = MOTOR_CALISIYOR;
      }
      break;
    
    case MOTOR_CALISIYOR:
      // Arka planı çalıştır
      useMotor(MOTOR_B);
      pulseAt(0, 0, 0);
      
      // Bitti mi?
      if (pulseAtBittiMi(MOTOR_B)) {
        Serial.println("Motor durdu!");
        durum = MOTOR_DURDU;
      }
      break;
    
    case MOTOR_DURDU:
      // Yeni iş yap...
      durum = BEKLIYOR;
      break;
  }
}

╔═══════════════════════════════════════════════════════════════╗
║                ÖRNEK 4: ACİL DURDURMA                          ║
╚═══════════════════════════════════════════════════════════════╝

void loop() {
  // Motorları çalıştır...
  if (pulseAtAktifMi(MOTOR_B)) {
    useMotor(MOTOR_B);
    pulseAt(0, 0, 0);
  }
  
  // Acil durdurma kontrolü (her zaman kontrol et)
  if (digitalRead(EMERGENCY_STOP_PIN) == LOW) {
    pulseAtHepsiniDurdur();
    Serial.println("ACİL DURDURMA!");
  }
}

*/

#endif // PULSEAT_H