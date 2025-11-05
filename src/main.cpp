// main.cpp - PEEK VERSION (DÜZELTILMIŞ)
// ═══════════════════════════════════════════════════════════════
// DÜZELTİLEN HATALAR:
//   ✅ bigEnc.reset() kullanıldı (setPosition değil)
//   ✅ a0FiltreliOku() kullanıldı (a0FiltreOku değil)
//   ✅ OPKON_PIN kullanıldı (A0_PIN değil)
// ═══════════════════════════════════════════════════════════════
#include "MoveSalinim.h" 
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
static long bigFreqMin = 10;
static long bigFreqMax = 100;
static long zEncMin = 0;
static long zEncMax = 20000;
static long bigFreqRef = 30;  // ✅ YENİ EKLENEN (eski BIG_FREQ_REF_HZ)

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
void handleX1Ayarla(const char* cmd);
void handleX2Ayarla(const char* cmd);
void handleXShow();
void handleBigRefAyarla(const char* cmd);
void handleBigRefShow();

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
  oynatmaRefHizSetup(&bigFreqRef);  // ✅ YENİ EKLE
  Serial.println(F("✓ Oynatma modülü hazır!\n"));
  
  Serial.println(F("[5/5] Çift Kayıt/Oynatma modülleri ayarlanıyor..."));
  ckEncoderSetup(&bigEnc, &xEnc);
  coEncoderSetup(&bigEnc, &xEnc, &zEnc);
  coParametreSetup(&bigFreqMin, &bigFreqMax, &zEncMin, &zEncMax);
  Serial.println(F("✓ Çift modüller hazır!\n"));

  // ✅ YENİ: MoveSalinim encoder setup
  msEncoderSetup(&xEnc);
  
  Serial.println(F("╔════════════════════════════════════════════════╗"));
  Serial.println(F("║            SİSTEM HAZIR! 🚀                    ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
  pinMode(KAYNAK_ROLE_PIN, OUTPUT);
  digitalWrite(KAYNAK_ROLE_PIN, HIGH); // NORMALDE AÇIK
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

  // ✅ YENİ: Salınım döngüsü (sadece TEK oynatma ile çalışır)
  if (oynatmaAktifMi() && !ckAktifMi()) {  // Güvenlik kontrolü
    msRun();
  }
  
  // Oynatma bitince salınımı durdur
  if (oynatmaTamamlandiMi() && msAktifMi()) {
    msDurdur();
    Serial.println("Oynatma bitti, salinim durduruldu!");
  }
  // ═══════════════════════════════════════════════════════════════
  // SERIAL OKUMA (Her zaman çalışıyor - blocking YOK!)
  // ═══════════════════════════════════════════════════════════════
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
// [BR] BIG REFERANS HIZ AYARLA
// ─────────────────────────────────────────────────────────────
  // [BR] BIG REFERANS HIZ AYARLA
  else if ((cmd[0] == 'B' || cmd[0] == 'b') && 
         (cmd[1] == 'R' || cmd[1] == 'r')) {
  // Eğer sadece "BR" ise göster, değilse ayarla.
    if (cmd[2] == '\0') {
     handleBigRefShow();  // "BR" → Göster
   } else {
      handleBigRefAyarla(cmd);  // "BR 50" veya "BR50" → Ayarla
    }
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
  else if ((cmd[0] == 'S' || cmd[0] == 's') && (cmd[1] == '\0')) {
    Serial.println(F("\n⚠️  ACİL DURDURMA!"));
    pulseAtHepsiniDurdur();
    moveToHepsiniDurdur();
    kayitDurdur();
    oynatmaDurdur();
    ckDurdur();
    coDurdur();
    msDurdur();
    digitalWrite(KAYNAK_ROLE_PIN,HIGH); // KAPALIYA AL
    Serial.println(F("✓ Tüm sistemler durduruldu!\n"));
  }

  else if ((cmd[0] == 'S' || cmd[0] == 's') && (cmd[1] == 'M' || cmd[1] == 'm')) {
    Serial.println(F("\n⚠️  ACİL MOTOR DURDURMA!"));
    pulseAtHepsiniDurdur();
    moveToHepsiniDurdur();
    Serial.println(F("✓ Tüm motorlar durduruldu!\n"));
  }
  
  // ─────────────────────────────────────────────────────────────
  // [E] ENCODER OKU
  // ─────────────────────────────────────────────────────────────
  else if (cmd[0] == 'E' || cmd[0] == 'e') {
    handleEncoderOku();
  }

  // ─────────────────────────────────────────────────────────
  else if (strcmp(cmd, "KL") == 0) {
    Serial.println(F("\n═══════════════════════════════════════"));
    Serial.println(F("  KAYIT1 VERİLERİ"));
    Serial.println(F("═══════════════════════════════════════"));
    ckKayit1Listele();
      
    Serial.println(F("\n═══════════════════════════════════════"));
    Serial.println(F("  KAYIT2 VERİLERİ"));
    Serial.println(F("═══════════════════════════════════════"));
    ckKayit2Listele();
      
    Serial.print(F("\nGlobal A0 Min: "));
    Serial.println(globalA0Min);
    Serial.print(F("Global A0 Max: "));
    Serial.println(globalA0Max);
  }
  
  // ─────────────────────────────────────────────────────────────
  // [A] A0 SENSÖR
  // ─────────────────────────────────────────────────────────────
  else if (cmd[0] == 'A' || cmd[0] == 'a') {
    handleA0Oku();
  }

  // ─────────────────────────────────────────────────────────
  else if (strcmp(cmd, "K") == 0) {
    digitalWrite(KAYNAK_ROLE_PIN, !digitalRead(KAYNAK_ROLE_PIN));
    Serial.println(F("[KAYNAK] Toggle"));
 }
  
  // ─────────────────────────────────────────────────────────────
  // [H] HELP/MENU
  // ─────────────────────────────────────────────────────────────
  else if (cmd[0] == 'H' || cmd[0] == 'h') {
    yazdirMenu();
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
  // [X1] X1 POZİSYON AYARLA
  // ─────────────────────────────────────────────────────────────
  else if ((cmd[0] == 'X' || cmd[0] == 'x') && 
           (cmd[1] == '1')) {
    handleX1Ayarla(cmd);
  }
  
  // ─────────────────────────────────────────────────────────────
  // [X2] X2 POZİSYON AYARLA
  // ─────────────────────────────────────────────────────────────
  else if ((cmd[0] == 'X' || cmd[0] == 'x') && 
           (cmd[1] == '2')) {
    handleX2Ayarla(cmd);
  }
  
  // ─────────────────────────────────────────────────────────────
  // [X SHOW] X POZİSYONLARI GÖSTER
  // ─────────────────────────────────────────────────────────────
  else if ((cmd[0] == 'X' || cmd[0] == 'x') && 
           (cmd[1] == ' ' || cmd[1] == '\0')) {
    handleXShow();
  }
  
  // ─────────────────────────────────────────────────────────────
  // BİLİNMEYEN KOMUT
  // ─────────────────────────────────────────────────────────────
  else {
    Serial.print(F("✗ Bilinmeyen komut: "));
    Serial.println(cmd);
    Serial.println(F("  'H' yazın menüyü görmek için."));
  }
}

// ═══════════════════════════════════════════════════════════════
// ENCODER OKU
// ═══════════════════════════════════════════════════════════════
void handleEncoderOku() {
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║              ENCODER POZİSYONLARI              ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝"));
  
  Serial.print(F("  Z (ENC2): "));
  Serial.println(zEnc.getPosition());
  
  Serial.print(F("  X (ENC1): "));
  Serial.println(xEnc.getPosition());
  
  Serial.print(F("  BIG (ENC3): "));
  Serial.println(bigEnc.getPosition());
  
  Serial.println();
}

// ═══════════════════════════════════════════════════════════════
// A0 SENSÖR OKU (DÜZELTILDI)
// ═══════════════════════════════════════════════════════════════
void handleA0Oku() {
  // ✅ DÜZELTME: a0FiltreliOku() kullan (a0FiltreOku değil)
  uint16_t filtrelenmis = a0FiltreliOku();
  
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║                A0 SENSÖR                       ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝"));
  
  Serial.print(F("  Filtreli : "));
  Serial.println(filtrelenmis);
  
  Serial.println();
}
// ═══════════════════════════════════════════════════════════════
// BIG REFERANS HIZ AYARLAMA
// ═══════════════════════════════════════════════════════════════
void handleBigRefAyarla(const char* cmd) {
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║        BIG REFERANS HIZ AYARLAMA               ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));

  // cmd başlangıcı: "BR..." -> argümanı BR'den sonraki kısımdan al, boşlukları at
  const char* arg = cmd + 2; // BR'den hemen sonra
  while (*arg == ' ' || *arg == '\t') arg++; // boşlukları at

  long yeniDeger;
  if (sscanf(arg, "%ld", &yeniDeger) == 1) {
    if (yeniDeger < 10 || yeniDeger > 500) {
      Serial.println(F("✗ Değer 10-500 arasında olmalı!"));
      Serial.println();
      return;
    }

    bigFreqRef = yeniDeger;

    Serial.println(F("✓ Referans hız güncellendi!"));
    Serial.print(F("  bigFreqRef = "));
    Serial.print(bigFreqRef);
    Serial.println(F(" Hz"));
  } else {
    Serial.println(F("✗ Geçersiz format!"));
    Serial.println(F("  Kullanım: BR 50 veya BR50"));
  }

  Serial.println();
}


// ═══════════════════════════════════════════════════════════════
// BIG REFERANS HIZ GÖSTER
// ═══════════════════════════════════════════════════════════════
void handleBigRefShow() {
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║         BIG REFERANS HIZ AYARI                 ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝"));
  
  Serial.print(F("  bigFreqRef: "));
  Serial.print(bigFreqRef);
  Serial.println(F(" Hz"));
  
  Serial.println(F("───────────────────────────────────────────────"));
  Serial.println(F("  Not: Depo kenarındaki (globalA0Min) hızdır."));
  Serial.println(F("       İçe doğru gidildikçe hız otomatik artar."));
  
  Serial.println();
}
// ═══════════════════════════════════════════════════════════════
// ÇİFT KAYIT BAŞLAT
// ═══════════════════════════════════════════════════════════════
void handleCiftKayit() {
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║             ÇİFT KAYIT BAŞLATMA                ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝"));
  
  Serial.println(F("\nMevcut X Pozisyonları:"));
  Serial.print(F("  x1Pos = "));
  Serial.println(x1Pos);
  Serial.print(F("  x2Pos = "));
  Serial.println(x2Pos);
  Serial.println(F("───────────────────────────────────────────────"));
  
  // Direkt başlat!
  ckBaslat(x1Pos, x2Pos, 0, 1);
}

// ═══════════════════════════════════════════════════════════════
// ÇİFT OYNATMA BAŞLAT
// ═══════════════════════════════════════════════════════════════
void handleCiftOynatma() {
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║            ÇİFT OYNATMA BAŞLATMA               ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝"));
  
  Serial.println(F("\nMevcut Parametreler:"));
  Serial.print(F("  x1Pos     = "));
  Serial.println(x1Pos);
  Serial.print(F("  x2Pos     = "));
  Serial.println(x2Pos);
  Serial.print(F("  BigFreqMin= "));
  Serial.println(bigFreqMin);
  Serial.print(F("  BigFreqMax= "));
  Serial.println(bigFreqMax);
  Serial.print(F("  zEncMin   = "));
  Serial.println(zEncMin);
  Serial.print(F("  zEncMax   = "));
  Serial.println(zEncMax);
  Serial.println(F("───────────────────────────────────────────────"));
  
  // Direkt başlat!
  coBaslat(x1Pos, x2Pos);
}

// ═══════════════════════════════════════════════════════════════
// RESET (ENCODER SIFIRLAMA) - DÜZELTILDI
// ═══════════════════════════════════════════════════════════════
void handleReset(char motor) {
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║            ENCODER SIFIRLAMA                   ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
  
  if (motor == 'Z' || motor == 'z') {
    long onceki = zEnc.getPosition();
    // ✅ DÜZELTME: reset() kullan (setPosition değil)
    zEnc.reset();
    
    Serial.print(F("  Z Encoder sıfırlandı! (Önceki: "));
    Serial.print(onceki);
    Serial.println(F(")"));
  }
  else if (motor == 'X' || motor == 'x') {
    long onceki = xEnc.getPosition();
    // ✅ DÜZELTME: reset() kullan (setPosition değil)
    xEnc.reset();
    
    Serial.print(F("  X Encoder sıfırlandı! (Önceki: "));
    Serial.print(onceki);
    Serial.println(F(")"));
  }
  else if (motor == 'B' || motor == 'b') {
    long onceki = bigEnc.getPosition();
    // ✅ DÜZELTME: reset() kullan (setPosition değil)
    bigEnc.reset();
    
    Serial.print(F("  BIG Encoder sıfırlandı! (Önceki: "));
    Serial.print(onceki);
    Serial.println(F(")"));
  }
  else {
    Serial.println(F("✗ Geçersiz motor! (Z/X/B)"));
  }
  
  Serial.println();
}

// ═══════════════════════════════════════════════════════════════
// X1 POZİSYON AYARLAMA
// ═══════════════════════════════════════════════════════════════
void handleX1Ayarla(const char* cmd) {
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║           X1 POZİSYON AYARLAMA                 ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
  
  // [DURUM 1] "X1 SET" → Mevcut encoder'ı al
  if (strstr(cmd, "SET") != nullptr || strstr(cmd, "set") != nullptr) {
    x1Pos = xEnc.getPosition();
    
    Serial.println(F("✓ X1 pozisyonu güncellendi!"));
    Serial.print(F("  x1Pos = "));
    Serial.println(x1Pos);
  }
  // [DURUM 2] "X1 5000" → Manuel değer
  else {
    long yeniDeger;
    if (sscanf(cmd + 3, "%ld", &yeniDeger) == 1) {
      x1Pos = yeniDeger;
      
      Serial.println(F("✓ X1 pozisyonu güncellendi!"));
      Serial.print(F("  x1Pos = "));
      Serial.println(x1Pos);
    }
    else {
      Serial.println(F("✗ Geçersiz format!"));
      Serial.println(F("  Kullanım: X1 SET  veya  X1 5000"));
    }
  }
  
  Serial.println();
}

// ═══════════════════════════════════════════════════════════════
// X2 POZİSYON AYARLAMA
// ═══════════════════════════════════════════════════════════════
void handleX2Ayarla(const char* cmd) {
  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║           X2 POZİSYON AYARLAMA                 ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));
  
  // [DURUM 1] "X2 SET" → Mevcut encoder'ı al
  if (strstr(cmd, "SET") != nullptr || strstr(cmd, "set") != nullptr) {
    x2Pos = xEnc.getPosition();
    
    Serial.println(F("✓ X2 pozisyonu güncellendi!"));
    Serial.print(F("  x2Pos = "));
    Serial.println(x2Pos);
  }
  // [DURUM 2] "X2 12000" → Manuel değer
  else {
    long yeniDeger;
    if (sscanf(cmd + 3, "%ld", &yeniDeger) == 1) {
      x2Pos = yeniDeger;
      
      Serial.println(F("✓ X2 pozisyonu güncellendi!"));
      Serial.print(F("  x2Pos = "));
      Serial.println(x2Pos);
    }
    else {
      Serial.println(F("✗ Geçersiz format!"));
      Serial.println(F("  Kullanım: X2 SET  veya  X2 12000"));
    }
  }
  
  Serial.println();
}

// ═══════════════════════════════════════════════════════════════
// X POZİSYONLARI GÖSTER
// ═══════════════════════════════════════════════════════════════
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

// ═══════════════════════════════════════════════════════════════
// MENÜ YAZDIR
// ═══════════════════════════════════════════════════════════════
void yazdirMenu() {
  Serial.println(F("╔════════════════════════════════════════════════╗"));
  Serial.println(F("║                ANA MENÜ                        ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝"));
  Serial.println(F("  MZ/MX/MB <hedef> <hz> → Motor hareket"));
  Serial.println(F("  KL                    → Kayıt1 & Kayıt2 listele"));
  Serial.println(F("  K                     → Kaynak röle toggle (Pin 14)"));
  Serial.println(F("  DZ/DX/DB              → Motor durdur"));
  Serial.println(F("  S                     → Acil durdur"));
  Serial.println(F("  SM                    → MOTORLARI ACİL DURDUR"));
  Serial.println(F("  E                     → Encoder oku"));
  Serial.println(F("  A                     → A0 sensör oku"));
  Serial.println(F("  RSTZ/RSTX/RSTB        → Encoder sıfırla"));
  Serial.println(F("  X1 SET / X1 <değer>   → X1 pozisyon ayarla"));
  Serial.println(F("  X2 SET / X2 <değer>   → X2 pozisyon ayarla"));
  Serial.println(F("  X                     → X pozisyonlarını göster"));
  Serial.println(F("  CK                    → Çift kayıt başlat"));
  Serial.println(F("  CO                    → Çift oynatma başlat"));
  Serial.println(F("  BR <değer> / BR       → Big referans hız ayarla/göster"));
  Serial.println(F("  H                     → Menü"));
  Serial.println(F("───────────────────────────────────────────────\n"));
}
