#ifndef CONFIG_H
#define CONFIG_H

// ═══════════════════════════════════════════════════════════════
// STEP MOTOR PİNLERİ
// ═══════════════════════════════════════════════════════════════

// Motor 1 (Z)
#define STEP1_PIN  5
#define DIR1_PIN   36
#define ENA1_PIN   37

// Motor 2 (X)
#define STEP2_PIN  6
#define DIR2_PIN   39
#define ENA2_PIN   38

// Motor 3 (BIG)
#define STEP3_PIN  7
#define DIR3_PIN   40
#define ENA3_PIN   41

// ═══════════════════════════════════════════════════════════════
// ENKODER PİNLERİ (Step Motorlar)
// ═══════════════════════════════════════════════════════════════
#define ENC1_A_PIN  3   // Motor X - interrupt destekli
#define ENC1_B_PIN  10

#define ENC2_A_PIN  2   // Motor Z - interrupt destekli
#define ENC2_B_PIN  9

#define ENC3_A_PIN  18  // Motor BIG - interrupt destekli
#define ENC3_B_PIN  11

// ═══════════════════════════════════════════════════════════════
// LİNEER POTANSİYOMETRE (OPKON)
// ═══════════════════════════════════════════════════════════════
#define OPKON_PIN   A0  // Analog giriş

// ═══════════════════════════════════════════════════════════════
// BUTONLAR
// ═══════════════════════════════════════════════════════════════
// Kalıcı (Toggle) Butonlar
#define TOGGLE1_PIN 22
#define TOGGLE2_PIN 23
#define TOGGLE3_PIN 24

// Push (Momentary) Butonlar
#define PUSH1_PIN   25
#define PUSH2_PIN   26
#define PUSH3_PIN   27

// Acil Durdurma (kullanılmıyor ama pin tanımlı)
#define EMERGENCY_STOP_PIN 28

// ═══════════════════════════════════════════════════════════════
// SD KART
// ═══════════════════════════════════════════════════════════════
#define SD_CS_PIN   53  // SPI Chip Select (Mega için sabit)

// ═══════════════════════════════════════════════════════════════
// KAYIT MODU PARAMETRELERİ
// ═══════════════════════════════════════════════════════════════
#define KAYIT_TOPLAM_PULSE   1600
#define KAYIT_ARALIK         160
#define KAYIT_HZ             80
//#define KAYIT_YON            0   //değişken oldugu için modul içine taşındı
#define KAYIT_ORNEK_SAYISI   (KAYIT_TOPLAM_PULSE / KAYIT_ARALIK + 1)

// ═══════════════════════════════════════════════════════════════
// MOTOR İNDEX TANIMLARI
// ═══════════════════════════════════════════════════════════════
#define MOTOR_Z  0  // 1. motor (Z ekseni)
#define MOTOR_X  1  // 2. motor (X ekseni)
#define MOTOR_B  2  // 3. motor (BIG motor)

// ═══════════════════════════════════════════════════════════════
// PIN DİZİLERİ (Motor index sırasına göre)
// ═══════════════════════════════════════════════════════════════
#define STEP_PINS { STEP1_PIN, STEP2_PIN, STEP3_PIN }
#define DIR_PINS  { DIR1_PIN,  DIR2_PIN,  DIR3_PIN  }
#define ENA_PINS  { ENA1_PIN,  ENA2_PIN,  ENA3_PIN  }

// ═══════════════════════════════════════════════════════════════
// PULSE AYARLARI
// ═══════════════════════════════════════════════════════════════
#define PULSE_HIGH_US  10  // Pulse HIGH süresi (mikrosaniye)

// ═══════════════════════════════════════════════════════════════
// ANALOG FİLTRE PARAMETRELERİ (A0 için)
// ═══════════════════════════════════════════════════════════════
#define A0_FILTER_SAMPLES      50   // Örnek sayısı (mod hesabı için)
#define A0_FILTER_SPACING_US   150  // Örnekler arası bekleme (µs)

// ═══════════════════════════════════════════════════════════════
// LİMİT SWITCH AYARLARI
// ═══════════════════════════════════════════════════════════════
// Limit switch pinleri (aktif LOW - pull-up ile)
#define LIMIT_Z_PIN   TOGGLE1_PIN  // 22 - Motor Z
#define LIMIT_X_PIN   TOGGLE2_PIN  // 23 - Motor X
#define LIMIT_B_PIN   TOGGLE3_PIN  // 24 - Motor BIG

// Home yönleri (0=ileri, 1=geri)
#define HOME_Z_DIR    1   // Z motoru geri giderek home'a ulaşır
#define HOME_X_DIR    0   // X motoru ileri giderek home'a ulaşır
#define HOME_B_DIR    1   // BIG motor geri giderek home'a ulaşır

// Home hızları (Hz)
#define HOME_Z_HZ     30
#define HOME_X_HZ     50
#define HOME_B_HZ     40

// Home maksimum pulse (güvenlik - sonsuz döngü önleme)
#define HOME_MAX_PULSE  10000

// ═══════════════════════════════════════════════════════════════
// MOTOR BAZLI İVME PARAMETRELERİ
// ═══════════════════════════════════════════════════════════════

// BIG Motor (Ağır yük, kısa mesafe)
#define ACCEL_RAMP_BIG   400    // Kısa rampa (hızlı ivmelenme)
#define MIN_SPEED_BIG    20     // Başlangıç hızı (Hz)

// X Motor (Hafif, uzun mesafe)
#define ACCEL_RAMP_X     5000   // Uzun rampa (yumuşak ivmelenme)
#define MIN_SPEED_X      100     // Düşük başlangıç (Hz)

// Z Motor (Torch, hassas)
#define ACCEL_RAMP_Z     5000   // Çok uzun rampa (çok yumuşak)
#define MIN_SPEED_Z      100     // Çok düşük başlangıç (Hz)

// ═══════════════════════════════════════════════════════════════
// NOTLAR
// ═══════════════════════════════════════════════════════════════
/*
SD KART BAĞLANTILARI (Arduino Mega):
  MOSI → Pin 51
  MISO → Pin 50
  SCK  → Pin 52
  CS   → Pin 53 (SD_CS_PIN)

ENCODER BAĞLANTILARI:
  - A fazı: Interrupt destekli pin (2, 3, 18, 19, 20, 21)
  - B fazı: Herhangi bir dijital pin
  
TB6600 SÜRÜCÜ:
  - ENA LOW  = Motor aktif
  - ENA HIGH = Motor devre dışı
  - DIR LOW  = İleri
  - DIR HIGH = Geri
*/

#endif // CONFIG_H