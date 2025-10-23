// main.cpp - ÇİFT KAYIT/OYNATMA + RESET ÖZELLİKLİ
// ═══════════════════════════════════════════════════════════════
// YENİ KOMUTLAR:
//   RSTZ  → Z encoder'ı sıfırla
//   RSTX  → X encoder'ı sıfırla  
//   RSTB  → BIG encoder'ı sıfırla
// ═══════════════════════════════════════════════════════════════

#include <Arduino.h>
#include "Config.h"
#include "PulseAt.h"
#include "MoveTo.h"
#include "stepmotorenkoderiokuma.h"
#include "A0Filtre.h"
#include "KayitModulu.h"
#include "OynatmaModulu.h"
#include "CiftKayitModulu.h"
#include "CiftOynatmaModulu.h"

// ═══════════════════════════════════════════════════════════════
// ENCODER NESNELERİ
// ═══════════════════════════════════════════════════════════════
StepMotorEncoder zEnc(ENC2_A_PIN, ENC2_B_PIN);
StepMotorEncoder xEnc(ENC1_A_PIN, ENC1_B_PIN);
StepMotorEncoder bigEnc(ENC3_A_PIN, ENC3_B_PIN);

// ═══════════════════════════════════════════════════════════════
// DİNAMİK PARAMETRELER
// ═══════════════════════════════════════════════════════════════
static long bigFreqMin = 50;
static long bigFreqMax = 100;
static long zEncMin = 0;
static long zEncMax = 20000;

// ═══════════════════════════════════════════════════════════════
// ÇİFT KAYIT/OYNATMA X POZİSYONLARI
// ═══════════════════════════════════════════════════════════════
static long x1Pos = 0;
static long x2Pos = -10000;

// ═══════════════════════════════════════════════════════════════
// KOMUT BUFFER
// ═══════════════════════════════════════════════════════════════
static char cmdBuffer[64];
static uint8_t cmdIndex = 0;

// ═══════════════════════════════════════════════════════════════
// FONKSİYON PROTOTIPLERI
// ═══════════════════════════════════════════════════════════════
void yazdirMenu();
void handleCommand(const char* cmd);
void handleEncoderOku();
void handleA0Oku();
void handleCiftKayit();
void handleCiftOynatma();
void handleReset(char motor);
void handleXAyarla(const char* cmd);
void handleXShow();
void handleX2Ayarla(const char* cmd);
void handleX1Ayarla(const char* cmd);

// ═══════════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║      DEPO KAYNAĞI SİSTEMİ BAŞLATILIYOR        ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
  
  Serial.println(F("[1/5] Encoder'lar başlatılıyor..."));
  zEnc.begin();
  xEnc.begin();
  bigEnc.begin();
  Serial.println(F("✓ Z, X, BIG encoder'lar hazır!\n"));
  
  Serial.println(F("[2/5] MoveTo modülü ayarlanıyor..."));
  moveToSetup(&zEnc, &xEnc, &bigEnc);
  Serial.println(F("✓ MoveTo hazır!\n"));
  
  Serial.println(F("[3/5] Kayıt modülü ayarlanıyor..."));
  kayitEncoderSetup(&bigEnc);
  Serial.println(F("✓ Kayıt modülü hazır!\n"));
  
  Serial.println(F("[4/5] Oynatma modülü ayarlanıyor..."));
  oynatmaEncoderSetup(&bigEnc, &zEnc);
  oynatmaParametreSetup(&bigFreqMin, &bigFreqMax, &zEncMin, &zEncMax);
  Serial.println(F("✓ Oynatma modülü hazır!\n"));
  
  Serial.println(F("[5/5] Çift Kayıt/Oynatma modülleri ayarlanıyor..."));
  ckEncoderSetup(&bigEnc, &xEnc);
  coEncoderSetup(&bigEnc, &xEnc, &zEnc);
  coParametreSetup(&bigFreqMin, &bigFreqMax, &zEncMin, &zEncMax);
  Serial.println(F("✓ Çift modüller hazır!\n"));
  
  Serial.println(F("╔════════════════════════════════════════════════╗"));
  Serial.println(F("║            SİSTEM HAZIR! 🚀                    ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
  
  yazdirMenu();
}

// ═══════════════════════════════════════════════════════════════
// LOOP
// ═══════════════════════════════════════════════════════════════
void loop() {
  moveToRun();
  kayitRun();
  oynatmaRun();
  ckRun();
  coRun();

  // MANUEL KOMUT ARKA PLANLARI (Sadece diğerleri aktif değilse)
  // ═══════════════════════════════════════════════════════════════
  
  // ─────────────────────────────────────────────────────────────
  // MOTOR Z - Manuel Pulse Arka Planı
  // ─────────────────────────────────────────────────────────────
  if (pulseAtAktifMi(MOTOR_Z)) {
    if (!moveToAktifMi(MOTOR_Z) && 
        !oynatmaAktifMi() && 
        !coAktifMi()) {
      useMotor(MOTOR_Z);
      pulseAt(0, 0, 0);
    }
  }
  
  // ─────────────────────────────────────────────────────────────
  // MOTOR X - Manuel Pulse Arka Planı
  // ─────────────────────────────────────────────────────────────
  if (pulseAtAktifMi(MOTOR_X)) {
    if (!moveToAktifMi(MOTOR_X) && 
        !ckAktifMi() && 
        !coAktifMi()) {
      useMotor(MOTOR_X);
      pulseAt(0, 0, 0);
    }
  }
  
  // ─────────────────────────────────────────────────────────────
  // MOTOR BIG - Manuel Pulse Arka Planı
  // ─────────────────────────────────────────────────────────────
  if (pulseAtAktifMi(MOTOR_B)) {
    if (!moveToAktifMi(MOTOR_B) && 
        !kayitAktifMi() && 
        !oynatmaAktifMi() && 
        !ckAktifMi() && 
        !coAktifMi()) {
      useMotor(MOTOR_B);
      pulseAt(0, 0, 0);
    }
  }
  
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (cmdIndex > 0) {
        cmdBuffer[cmdIndex] = '\0';
        handleCommand(cmdBuffer);
        cmdIndex = 0;
      }
    }
    else if (cmdIndex < sizeof(cmdBuffer) - 1) {
      cmdBuffer[cmdIndex++] = c;
    }
  }
}

// ═══════════════════════════════════════════════════════════════
// KOMUT İŞLEYİCİSİ
// ═══════════════════════════════════════════════════════════════
void handleCommand(const char* cmd) {
  
  // ─────────────────────────────────────────────────────────────
  // [RST] RESET KOMUTU: RSTZ/RSTX/RSTB
  // ─────────────────────────────────────────────────────────────
  if ((cmd[0] == 'R' || cmd[0] == 'r') &&
      (cmd[1] == 'S' || cmd[1] == 's') &&
      (cmd[2] == 'T' || cmd[2] == 't')) {
    char motor = cmd[3];
    handleReset(motor);
    return;
  }
  
  // ─────────────────────────────────────────────────────────────
  // [M] MOVETO: MZ/MX/MB hedef hz
  // ─────────────────────────────────────────────────────────────
  else if (cmd[0] == 'M' || cmd[0] == 'm') {
    char motor = cmd[1];
    uint8_t motorIndex;
    
    if (motor == 'Z' || motor == 'z') motorIndex = MOTOR_Z;
    else if (motor == 'X' || motor == 'x') motorIndex = MOTOR_X;
    else if (motor == 'B' || motor == 'b') motorIndex = MOTOR_B;
    else {
      Serial.println(F("✗ Geçersiz motor! (MZ/MX/MB)"));
      return;
    }
    
    long hedef;
    unsigned int hz;
    
    if (sscanf(cmd + 2, "%ld %u", &hedef, &hz) == 2) {
      if (hz == 0) {
        Serial.println(F("✗ Hz 0 olamaz!"));
        return;
      }
      
      Serial.print(F("[M"));
      Serial.print(motor);
      Serial.print(F("] "));
      Serial.print(hedef);
      Serial.print(F(" @ "));
      Serial.print(hz);
      Serial.print(F("Hz → "));
      
      if (moveTo(motorIndex, hedef, hz)) {
        Serial.println(F("✓"));
      } else {
        Serial.println(F("✗ (Aktif)"));
      }
    } else {
      Serial.println(F("✗ Format: MZ hedef hz"));
    }
  }
  
  // ─────────────────────────────────────────────────────────────
  // [P] PULSEAT: PZ/PX/PB pulse yon hz
  // ─────────────────────────────────────────────────────────────
  else if (cmd[0] == 'P' || cmd[0] == 'p') {
    char motor = cmd[1];
    uint8_t motorIndex;
    
    if (motor == 'Z' || motor == 'z') motorIndex = MOTOR_Z;
    else if (motor == 'X' || motor == 'x') motorIndex = MOTOR_X;
    else if (motor == 'B' || motor == 'b') motorIndex = MOTOR_B;
    else {
      Serial.println(F("✗ Geçersiz motor! (PZ/PX/PB)"));
      return;
    }
    
    unsigned long pulse;
    int yon;
    unsigned int hz;
    
    if (sscanf(cmd + 2, "%lu %d %u", &pulse, &yon, &hz) == 3) {
      if (pulse == 0 || hz == 0) {
        Serial.println(F("✗ Pulse ve Hz 0'dan büyük olmalı!"));
        return;
      }
      
      Serial.print(F("[P"));
      Serial.print(motor);
      Serial.print(F("] "));
      Serial.print(pulse);
      Serial.print(F("p "));
      Serial.print(yon ? F("←") : F("→"));
      Serial.print(F(" "));
      Serial.print(hz);
      Serial.println(F("Hz ✓"));
      
      useMotor(motorIndex);
      pulseAt(pulse, yon, hz);
    } else {
      Serial.println(F("✗ Format: PZ pulse yon hz"));
    }
  }
  
  // ─────────────────────────────────────────────────────────────
  // [D] DURDUR: DZ/DX/DB
  // ─────────────────────────────────────────────────────────────
  else if (cmd[0] == 'D' || cmd[0] == 'd') {
    char motor = cmd[1];
    uint8_t motorIndex;
    
    if (motor == 'Z' || motor == 'z') motorIndex = MOTOR_Z;
    else if (motor == 'X' || motor == 'x') motorIndex = MOTOR_X;
    else if (motor == 'B' || motor == 'b') motorIndex = MOTOR_B;
    else {
      Serial.println(F("✗ Geçersiz motor! (DZ/DX/DB)"));
      return;
    }
    
    pulseAtDurdur(motorIndex);
    moveToDurdur(motorIndex);
    
    Serial.print(F("[D"));
    Serial.print(motor);
    Serial.println(F("] ✓ Durdu"));
  }
  
  // ─────────────────────────────────────────────────────────────
  // [S] ACİL DURDURMA
  // ─────────────────────────────────────────────────────────────
  else if (cmd[0] == 'S' || cmd[0] == 's') {
    Serial.println(F("\n⚠️  ACİL DURDURMA!"));
    pulseAtHepsiniDurdur();
    moveToHepsiniDurdur();
    kayitDurdur();
    oynatmaDurdur();
    ckDurdur();
    coDurdur();
    Serial.println(F("✓ Tüm sistemler durduruldu!\n"));
  }
  
  // ─────────────────────────────────────────────────────────────
  // [E] ENCODER OKU
  // ─────────────────────────────────────────────────────────────
  else if (cmd[0] == 'E' || cmd[0] == 'e') {
    handleEncoderOku();
  }
  
  // ─────────────────────────────────────────────────────────────
  // [A] A0 SENSÖR
  // ─────────────────────────────────────────────────────────────
  else if (cmd[0] == 'A' || cmd[0] == 'a') {
    handleA0Oku();
  }
  
  // ─────────────────────────────────────────────────────────────
  // [CK] ÇİFT KAYIT
  // ─────────────────────────────────────────────────────────────
  else if ((cmd[0] == 'C' || cmd[0] == 'c') && 
           (cmd[1] == 'K' || cmd[1] == 'k')) {
    handleCiftKayit();
  }
  
  // ─────────────────────────────────────────────────────────────
  // [CO] ÇİFT OYNATMA
  // ─────────────────────────────────────────────────────────────
  else if ((cmd[0] == 'C' || cmd[0] == 'c') && 
           (cmd[1] == 'O' || cmd[1] == 'o')) {
    handleCiftOynatma();
  }
  
  // ─────────────────────────────────────────────────────────────
  // [C1] KAYIT1 LİSTELE
  // ─────────────────────────────────────────────────────────────
  else if ((cmd[0] == 'C' || cmd[0] == 'c') && cmd[1] == '1') {
    if (ckTamamlandiMi()) {
      ckKayit1Listele();
    } else {
      Serial.println(F("✗ Kayıt yok!"));
    }
  }
  
  // ─────────────────────────────────────────────────────────────
  // [C2] KAYIT2 LİSTELE
  // ─────────────────────────────────────────────────────────────
  else if ((cmd[0] == 'C' || cmd[0] == 'c') && cmd[1] == '2') {
    if (ckTamamlandiMi()) {
      ckKayit2Listele();
    } else {
      Serial.println(F("✗ Kayıt yok!"));
    }
  }
  
  // ─────────────────────────────────────────────────────────────
  // [H] YARDIM
  // ─────────────────────────────────────────────────────────────
  else if (cmd[0] == 'H' || cmd[0] == 'h' || cmd[0] == '?') {
    yazdirMenu();
  }
  
  
  
  // ─────────────────────────────────────────────────────────────
  // [X1] X1 POZİSYON AYARLAMA
  // ─────────────────────────────────────────────────────────────
  else if (strncmp(cmd, "X1", 2) == 0) {
    handleX1Ayarla(cmd);
  }
  
  // ─────────────────────────────────────────────────────────────
  // [X2] X2 POZİSYON AYARLAMA
  // ─────────────────────────────────────────────────────────────
  else if (strncmp(cmd, "X2", 2) == 0) {
    handleX2Ayarla(cmd);
  }
  
  // ─────────────────────────────────────────────────────────────
  // [X SHOW] X POZİSYONLARINI GÖSTER
  // ─────────────────────────────────────────────────────────────
  else if (strcmp(cmd, "X SHOW") == 0 || strcmp(cmd, "XSHOW") == 0) {
    handleXShow();
  }
  // ─────────────────────────────────────────────────────────────
  // BİLİNMEYEN KOMUT
  // ─────────────────────────────────────────────────────────────
  else {
    Serial.print(F("✗ Bilinmeyen: "));
    Serial.println(cmd);
  }
}

// ═══════════════════════════════════════════════════════════════
// RESET İŞLEYİCİSİ
// ═══════════════════════════════════════════════════════════════
void handleReset(char motor) {
  if (motor == 'Z' || motor == 'z') {
    zEnc.reset();
    Serial.println(F("[RSTZ] ✓ Z encoder sıfırlandı (0)"));
  }
  else if (motor == 'X' || motor == 'x') {
    xEnc.reset();
    Serial.println(F("[RSTX] ✓ X encoder sıfırlandı (0)"));
  }
  else if (motor == 'B' || motor == 'b') {
    bigEnc.reset();
    Serial.println(F("[RSTB] ✓ BIG encoder sıfırlandı (0)"));
  }
  else {
    Serial.println(F("✗ Geçersiz motor! (RSTZ/RSTX/RSTB)"));
  }
}

// ═══════════════════════════════════════════════════════════════
// MENÜ
// ═══════════════════════════════════════════════════════════════
void yazdirMenu() {
  Serial.println(F("╔════════════════════════════════════════════════╗"));
  Serial.println(F("║              KOMUT LİSTESİ                     ║"));
  Serial.println(F("╠════════════════════════════════════════════════╣"));
  Serial.println(F("║ MOTOR KOMUTLARI                                ║"));
  Serial.println(F("║───────────────────────────────────────────────║"));
  Serial.println(F("║  MZ 500 50     → Z motor 500'e 50Hz            ║"));
  Serial.println(F("║  MX 12000 100  → X motor 12000'e 100Hz         ║"));
  Serial.println(F("║  MB 5000 80    → BIG motor 5000'e 80Hz         ║"));
  Serial.println(F("║                                                ║"));
  Serial.println(F("║  PZ 1000 0 50  → Z: 1000p ileri 50Hz           ║"));
  Serial.println(F("║  PB 500 1 30   → BIG: 500p geri 30Hz           ║"));
  Serial.println(F("║                                                ║"));
  Serial.println(F("║  DZ            → Z motorunu durdur             ║"));
  Serial.println(F("║  S             → ACİL DURDURMA (hepsi)         ║"));
  Serial.println(F("╠════════════════════════════════════════════════╣"));
  Serial.println(F("║ ENCODER İŞLEMLERİ                              ║"));
  Serial.println(F("║───────────────────────────────────────────────║"));
  Serial.println(F("║  E             → Encoder pozisyonları          ║"));
  Serial.println(F("║  RSTZ          → Z encoder'ı sıfırla (0)       ║"));
  Serial.println(F("║  RSTX          → X encoder'ı sıfırla (0)       ║"));
  Serial.println(F("║  RSTB          → BIG encoder'ı sıfırla (0)     ║"));
  Serial.println(F("╠════════════════════════════════════════════════╣"));
  Serial.println(F("║ BİLGİ                                          ║"));
  Serial.println(F("║───────────────────────────────────────────────║"));
  Serial.println(F("║  A             → A0 sensör oku                 ║"));
  Serial.println(F("╠════════════════════════════════════════════════╣"));
  Serial.println(F("║ ÇİFT KAYIT/OYNATMA                             ║"));
  Serial.println(F("║───────────────────────────────────────────────║"));
  Serial.println(F("║  CK            → Çift Kayıt başlat             ║"));
  Serial.println(F("║  CO            → Çift Oynatma başlat           ║"));
  Serial.println(F("║  C1            → Kayıt1 listele                ║"));
  Serial.println(F("║  C2            → Kayıt2 listele                ║"));
  Serial.println(F("╠════════════════════════════════════════════════╣"));
  Serial.println(F("║ X POZİSYON AYARLAMA                            ║"));
  Serial.println(F("║───────────────────────────────────────────────║"));
  Serial.println(F("║  X1 SET        → Mevcut X'i x1Pos yap          ║"));
  Serial.println(F("║  X1 5000       → x1Pos'u 5000 yap              ║"));
  Serial.println(F("║  X2 SET        → Mevcut X'i x2Pos yap          ║"));
  Serial.println(F("║  X2 12000      → x2Pos'u 12000 yap             ║"));
  Serial.println(F("║  X SHOW        → x1Pos ve x2Pos'u göster       ║"));
  Serial.println(F("╠════════════════════════════════════════════════╣"));
  Serial.println(F("║  H veya ?      → Bu menü                       ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
}

// ═══════════════════════════════════════════════════════════════
// ENCODER OKU
// ═══════════════════════════════════════════════════════════════
void handleEncoderOku() {
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║          ENCODER POZİSYONLARI                  ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝"));
  Serial.print(F("  Z  : "));
  Serial.println(zEnc.getPosition());
  Serial.print(F("  X  : "));
  Serial.println(xEnc.getPosition());
  Serial.print(F("  BIG: "));
  Serial.println(bigEnc.getPosition());
  Serial.println();
}

// ═══════════════════════════════════════════════════════════════
// A0 SENSÖR
// ═══════════════════════════════════════════════════════════════
void handleA0Oku() {
  Serial.print(F("\n[A0] Okuma... "));
  uint16_t val = a0FiltreliOku();
  Serial.print(val);
  Serial.println(F(" (0-1023)\n"));
}

// ═══════════════════════════════════════════════════════════════
// ÇİFT KAYIT İŞLEYİCİSİ
// ═══════════════════════════════════════════════════════════════
void handleCiftKayit() {
  if (ckAktifMi() || coAktifMi() || kayitAktifMi() || oynatmaAktifMi()) {
    Serial.println(F("\n✗ Başka işlem aktif! 'S' ile durdur.\n"));
    return;
  }
  
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║          ÇİFT KAYIT PARAMETRELERİ              ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝"));
  Serial.print(F("  X1 Pozisyon : "));
  Serial.println(x1Pos);
  Serial.print(F("  X2 Pozisyon : "));
  Serial.println(x2Pos);
  Serial.println(F("  Kayıt1      : İleri (BIG: 0→16000)"));
  Serial.println(F("  Kayıt2      : Geri  (BIG: 16000→0)"));
  Serial.println(F("───────────────────────────────────────────────"));
  
  // ✅ BUFFER TEMİZLE (soru sormadan ÖNCE!)
  while (Serial.available() > 0) {
    Serial.read();
  }
  
  Serial.print(F("Başlat? (Y/N): "));
  
  unsigned long t = millis();
  while (Serial.available() == 0 && millis() - t < 30000) delay(10);
  
  if (Serial.available() > 0) {
    char c = Serial.read();
    Serial.println(c);
    
    if (c == 'Y' || c == 'y') {
      ckBaslat(x1Pos, x2Pos, 0, 1);
      Serial.println(F("\n✓ Çift kayıt başlatıldı!"));
      Serial.println(F("═══════════════════════════════════════════════"));
      Serial.println(F("ADIM 1/7: X1 pozisyonuna gidiliyor..."));
    } else {
      Serial.println(F("✗ İptal\n"));
    }
  } else {
    Serial.println(F("\n✗ Timeout\n"));
  }
}

// ═══════════════════════════════════════════════════════════════
// ÇİFT OYNATMA İŞLEYİCİSİ
// ═══════════════════════════════════════════════════════════════
void handleCiftOynatma() {
  if (!ckTamamlandiMi()) {
    Serial.println(F("\n✗ Kayıt yok! Önce CK komutu.\n"));
    return;
  }
  
  if (coAktifMi() || kayitAktifMi() || oynatmaAktifMi()) {
    Serial.println(F("\n✗ Başka işlem aktif!\n"));
    return;
  }
  
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║         ÇİFT OYNATMA PARAMETRELERİ             ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝"));
  Serial.print(F("  X1 Pozisyon : "));
  Serial.println(x1Pos);
  Serial.print(F("  X2 Pozisyon : "));
  Serial.println(x2Pos);
  Serial.print(F("  A0 Min      : "));
  Serial.println(ckGlobalA0Min());
  Serial.print(F("  A0 Max      : "));
  Serial.println(ckGlobalA0Max());
  Serial.print(F("  BIG Freq    : "));
  Serial.print(bigFreqMin);
  Serial.print(F("-"));
  Serial.println(bigFreqMax);
  Serial.print(F("  Z Enc Max   : "));
  Serial.println(zEncMax);
  Serial.println(F("───────────────────────────────────────────────"));
  
  // ✅ BUFFER TEMİZLE (soru sormadan ÖNCE!)
  while (Serial.available() > 0) {
    Serial.read();
  }
  
  Serial.print(F("Başlat? (Y/N): "));
  
  unsigned long t = millis();
  while (Serial.available() == 0 && millis() - t < 30000) delay(10);
  
  if (Serial.available() > 0) {
    char c = Serial.read();
    Serial.println(c);
    
    if (c == 'Y' || c == 'y') {
      coBaslat(x1Pos, x2Pos);
      Serial.println(F("\n✓ Çift oynatma başlatıldı!"));
      Serial.println(F("═══════════════════════════════════════════════"));
      Serial.println(F("ADIM 1/7: X1 pozisyonuna gidiliyor..."));
    } else {
      Serial.println(F("✗ İptal\n"));
    }
  } else {
    Serial.println(F("\n✗ Timeout\n"));
  }
}


// ═══════════════════════════════════════════════════════════════
// X POZİSYON AYARLAMA FONKSİYONLARI
// ═══════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────
// [X1 SET] → Mevcut X encoder'ı x1Pos olarak kaydet
// [X1 5000] → x1Pos'u 5000 olarak ayarla
// ─────────────────────────────────────────────────────────────
void handleX1Ayarla(const char* cmd) {
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║           X1 POZİSYON AYARLAMA                 ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
  
  // [DURUM 1] "X1 SET" komutu → Mevcut encoder'ı al
  if (strstr(cmd, "SET") != nullptr) {
    long mevcutX = xEnc.getPosition();
    
    Serial.print(F("  Mevcut X encoder: "));
    Serial.println(mevcutX);
    
    // ✅ BUFFER TEMİZLE (soru sormadan ÖNCE!)
    while (Serial.available() > 0) {
      Serial.read();
    }
    
    Serial.print(F("\n  X1 pozisyonunu bu değere ayarla? (Y/N): "));
    
    // [ONAY BEKLE]
    unsigned long baslangic = millis();
    while (Serial.available() == 0 && millis() - baslangic < 30000) {
      delay(10);
    }
    
    if (Serial.available() > 0) {
      char onay = Serial.read();
      Serial.println(onay);
      
      if (onay == 'Y' || onay == 'y') {
        x1Pos = mevcutX;
        
        Serial.println(F("\n✓ X1 pozisyonu güncellendi!"));
        Serial.print(F("  x1Pos = "));
        Serial.println(x1Pos);
      }
      else {
        Serial.println(F("\n✗ İptal edildi."));
      }
    }
    else {
      Serial.println(F("\n✗ Timeout!"));
    }
  }
  
  // [DURUM 2] "X1 5000" komutu → Manuel değer gir
  else {
    long yeniDeger;
    
    // "X1 " kısmını atla, sayıyı al
    if (sscanf(cmd + 3, "%ld", &yeniDeger) == 1) {
      Serial.print(F("  Yeni X1 pozisyonu: "));
      Serial.println(yeniDeger);
      
      // ✅ BUFFER TEMİZLE (soru sormadan ÖNCE!)
      while (Serial.available() > 0) {
        Serial.read();
      }
      
      Serial.print(F("\n  X1'i bu değere ayarla? (Y/N): "));
      
      // [ONAY BEKLE]
      unsigned long baslangic = millis();
      while (Serial.available() == 0 && millis() - baslangic < 30000) {
        delay(10);
      }
      
      if (Serial.available() > 0) {
        char onay = Serial.read();
        Serial.println(onay);
        
        if (onay == 'Y' || onay == 'y') {
          x1Pos = yeniDeger;
          
          Serial.println(F("\n✓ X1 pozisyonu güncellendi!"));
          Serial.print(F("  x1Pos = "));
          Serial.println(x1Pos);
        }
        else {
          Serial.println(F("\n✗ İptal edildi."));
        }
      }
      else {
        Serial.println(F("\n✗ Timeout!"));
      }
    }
    else {
      Serial.println(F("✗ Geçersiz format!"));
      Serial.println(F("  Kullanım: X1 SET  veya  X1 5000"));
    }
  }
  
  Serial.println();
}

// ─────────────────────────────────────────────────────────────
// [X2 SET] → Mevcut X encoder'ı x2Pos olarak kaydet
// [X2 12000] → x2Pos'u 12000 olarak ayarla
// ─────────────────────────────────────────────────────────────
void handleX2Ayarla(const char* cmd) {
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║           X2 POZİSYON AYARLAMA                 ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
  
  // [DURUM 1] "X2 SET" komutu → Mevcut encoder'ı al
  if (strstr(cmd, "SET") != nullptr) {
    long mevcutX = xEnc.getPosition();
    
    Serial.print(F("  Mevcut X encoder: "));
    Serial.println(mevcutX);
    
    // ✅ BUFFER TEMİZLE (soru sormadan ÖNCE!)
    while (Serial.available() > 0) {
      Serial.read();
    }
    
    Serial.print(F("\n  X2 pozisyonunu bu değere ayarla? (Y/N): "));
    
    // [ONAY BEKLE]
    unsigned long baslangic = millis();
    while (Serial.available() == 0 && millis() - baslangic < 30000) {
      delay(10);
    }
    
    if (Serial.available() > 0) {
      char onay = Serial.read();
      Serial.println(onay);
      
      if (onay == 'Y' || onay == 'y') {
        x2Pos = mevcutX;
        
        Serial.println(F("\n✓ X2 pozisyonu güncellendi!"));
        Serial.print(F("  x2Pos = "));
        Serial.println(x2Pos);
      }
      else {
        Serial.println(F("\n✗ İptal edildi."));
      }
    }
    else {
      Serial.println(F("\n✗ Timeout!"));
    }
  }
  
  // [DURUM 2] "X2 12000" komutu → Manuel değer gir
  else {
    long yeniDeger;
    
    // "X2 " kısmını atla, sayıyı al
    if (sscanf(cmd + 3, "%ld", &yeniDeger) == 1) {
      Serial.print(F("  Yeni X2 pozisyonu: "));
      Serial.println(yeniDeger);
      
      // ✅ BUFFER TEMİZLE (soru sormadan ÖNCE!)
      while (Serial.available() > 0) {
        Serial.read();
      }
      
      Serial.print(F("\n  X2'yi bu değere ayarla? (Y/N): "));
      
      // [ONAY BEKLE]
      unsigned long baslangic = millis();
      while (Serial.available() == 0 && millis() - baslangic < 30000) {
        delay(10);
      }
      
      if (Serial.available() > 0) {
        char onay = Serial.read();
        Serial.println(onay);
        
        if (onay == 'Y' || onay == 'y') {
          x2Pos = yeniDeger;
          
          Serial.println(F("\n✓ X2 pozisyonu güncellendi!"));
          Serial.print(F("  x2Pos = "));
          Serial.println(x2Pos);
        }
        else {
          Serial.println(F("\n✗ İptal edildi."));
        }
      }
      else {
        Serial.println(F("\n✗ Timeout!"));
      }
    }
    else {
      Serial.println(F("✗ Geçersiz format!"));
      Serial.println(F("  Kullanım: X2 SET  veya  X2 12000"));
    }
  }
  
  Serial.println();
}

// ─────────────────────────────────────────────────────────────
// [X SHOW] → x1Pos ve x2Pos'u göster
// ─────────────────────────────────────────────────────────────
void handleXShow() {
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║          X POZİSYON AYARLARI                   ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝"));
  
  Serial.print(F("  x1Pos (Kayıt1): "));
  Serial.println(x1Pos);
  
  Serial.print(F("  x2Pos (Kayıt2): "));
  Serial.println(x2Pos);
  
  Serial.println(F("───────────────────────────────────────────────"));
  Serial.print(F("  Mevcut X encoder: "));
  Serial.println(xEnc.getPosition());
  
  Serial.println();
}