/*
 * ═══════════════════════════════════════════════════════════════
 *  TAM SİSTEM ENTEGRASYON TESTİ
 * ═══════════════════════════════════════════════════════════════
 *
 * ÖZELLİKLER:
 *   ✓ Çoklu segment yönetimi (3 malzeme)
 *   ✓ SD kart kayıt/okuma
 *   ✓ Röle kontrolü (kaynak makinesi)
 *   ✓ Delta ekran simülasyonu
 *   ✓ Limit switch + Emergency stop
 *   ✓ Homing sistemi
 *
 * KULLANIM:
 *   platformio.ini'de test environment'ı aktif et
 *   Seri monitör: 115200 baud
 *   Komutlar için: HELP
 * ═══════════════════════════════════════════════════════════════
 */

#include <Arduino.h>
#include <SD.h>
#include "Config.h"
#include "PulseAt.h"
#include "KayitModulu.h"
#include "OynatmaModulu.h"
#include "stepmotorenkoderiokuma.h"
#include "HomingModulu.h"

// ═══════════════════════════════════════════════════════════════
// RÖLE PİNİ (Kaynak makinesi kontrol)
// ═══════════════════════════════════════════════════════════════
#define ROLE_PIN  45  // Boş bir dijital pin

// ═══════════════════════════════════════════════════════════════
// DELTA EKRAN HABERLEŞMESİ (Serial1 veya Serial2)
// ═══════════════════════════════════════════════════════════════
#define DELTA_SERIAL Serial1  // Arduino Mega Serial1 (TX1: 18, RX1: 19)
#define DELTA_BAUD 9600

// ═══════════════════════════════════════════════════════════════
// SEGMENT YÖNETİMİ
// ═══════════════════════════════════════════════════════════════
#define MAX_SEGMENTS 5  // Maksimum 5 segment

struct SegmentInfo {
  bool     var = false;          // Segment kayıtlı mı?
  char     dosyaAdi[20];         // "seg_1.txt"
  char     malzemeAdi[30];       // "Alüminyum"
  uint16_t ornekSayisi = 0;      // Kaç örnek var?
  int      a0Min = 1023;         // A0 minimum
  int      a0Max = 0;            // A0 maksimum
};

SegmentInfo segments[MAX_SEGMENTS];
uint8_t aktifSegment = 0;  // Şu an seçili segment (0-4)

// ═══════════════════════════════════════════════════════════════
// ENCODER NESNELERİ
// ═══════════════════════════════════════════════════════════════
StepMotorEncoder encZ(ENC1_A_PIN, ENC1_B_PIN);
StepMotorEncoder encX(ENC2_A_PIN, ENC2_B_PIN);
StepMotorEncoder encB(ENC3_A_PIN, ENC3_B_PIN);

// ═══════════════════════════════════════════════════════════════
// SİSTEM DURUMU
// ═══════════════════════════════════════════════════════════════
enum SystemMode {
  MODE_IDLE,
  MODE_HOMING,
  MODE_KAYIT_SEGMENT,
  MODE_OYNATMA_SEGMENT
};

SystemMode sistemModu = MODE_IDLE;
String komutBuffer = "";
bool yeniKomutVar = false;
int toplamKomut = 0;

// Röle durumu
bool roleAktif = false;

// ═══════════════════════════════════════════════════════════════
// RÖLE FONKSİYONLARI
// ═══════════════════════════════════════════════════════════════
void roleAc() {
  digitalWrite(ROLE_PIN, HIGH);
  roleAktif = true;
  Serial.println(F("\n🔥 RÖLE AÇIK (Kaynak başladı)"));

  // Delta ekrana bildir
  DELTA_SERIAL.println(F("EVENT:ROLE_ON"));
}

void roleKapat() {
  digitalWrite(ROLE_PIN, LOW);
  roleAktif = false;
  Serial.println(F("\n❄️  RÖLE KAPALI (Kaynak durdu)"));

  // Delta ekrana bildir
  DELTA_SERIAL.println(F("EVENT:ROLE_OFF"));
}

// ═══════════════════════════════════════════════════════════════
// DELTA EKRAN FONKSİYONLARI
// ═══════════════════════════════════════════════════════════════
void deltaGonderDurum() {
  // FORMAT: STATUS:<MOD>:<SEGMENT>:<PROGRESS>:<ENC_B>:<ENC_X>

  DELTA_SERIAL.print(F("STATUS:"));

  switch (sistemModu) {
    case MODE_IDLE:            DELTA_SERIAL.print(F("IDLE")); break;
    case MODE_HOMING:          DELTA_SERIAL.print(F("HOME")); break;
    case MODE_KAYIT_SEGMENT:   DELTA_SERIAL.print(F("KAYIT")); break;
    case MODE_OYNATMA_SEGMENT: DELTA_SERIAL.print(F("OYNAT")); break;
  }

  DELTA_SERIAL.print(F(":"));
  DELTA_SERIAL.print(aktifSegment);
  DELTA_SERIAL.print(F(":"));
  DELTA_SERIAL.print(0);  // Progress (TODO: gerçek değer)
  DELTA_SERIAL.print(F(":"));
  DELTA_SERIAL.print(encB.getPosition());
  DELTA_SERIAL.print(F(":"));
  DELTA_SERIAL.println(encX.getPosition());
}

void deltaOkuKomut() {
  // FORMAT: CMD:<KOMUT>:<PARAM>

  if (DELTA_SERIAL.available() > 0) {
    String cmd = DELTA_SERIAL.readStringUntil('\n');
    cmd.trim();

    Serial.print(F("\n[DELTA] \""));
    Serial.print(cmd);
    Serial.println(F("\""));

    // Parse: "CMD:START_KAYIT:1"
    if (cmd.startsWith("CMD:")) {
      cmd = cmd.substring(4);  // "START_KAYIT:1"

      int colonPos = cmd.indexOf(':');
      String komut = cmd.substring(0, colonPos);     // "START_KAYIT"
      String paramStr = cmd.substring(colonPos + 1); // "1"
      int param = paramStr.toInt();

      // Komut işle
      if (komut == "START_KAYIT") {
        aktifSegment = param;
        Serial.print(F(">> Delta: Segment "));
        Serial.print(param);
        Serial.println(F(" kayıt başlat"));
        // TODO: Kayıt başlat
      }
      else if (komut == "START_OYNAT") {
        aktifSegment = param;
        Serial.print(F(">> Delta: Segment "));
        Serial.print(param);
        Serial.println(F(" oynat"));
        // TODO: Oynat başlat
      }
      else if (komut == "STOP") {
        Serial.println(F(">> Delta: Durdur komutu"));
        komutBuffer = "STOP";
        yeniKomutVar = true;
      }
      else if (komut == "HOME") {
        Serial.println(F(">> Delta: Homing komutu"));
        komutBuffer = "HOME";
        yeniKomutVar = true;
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════════
// SD KART SEGMENT FONKSİYONLARI
// ═══════════════════════════════════════════════════════════════
bool sdBaslat() {
  Serial.print(F(">> SD kart başlatılıyor..."));

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F(" ❌ BAŞARISIZ!"));
    return false;
  }

  Serial.println(F(" ✓"));

  // Segments klasörü var mı?
  if (!SD.exists("/segments")) {
    Serial.println(F(">> /segments klasörü oluşturuluyor..."));
    SD.mkdir("/segments");
  }

  return true;
}

void sdSegmentleriYukle() {
  Serial.println(F("\n>> SD karttan segmentler yükleniyor..."));

  for (uint8_t i = 0; i < MAX_SEGMENTS; i++) {
    sprintf(segments[i].dosyaAdi, "/segments/seg_%d.txt", i + 1);

    if (SD.exists(segments[i].dosyaAdi)) {
      File f = SD.open(segments[i].dosyaAdi, FILE_READ);
      if (f) {
        // İlk satır: Malzeme adı
        if (f.available()) {
          String malzeme = f.readStringUntil('\n');
          malzeme.trim();
          malzeme.toCharArray(segments[i].malzemeAdi, 30);
        }

        // Satır sayısını hesapla
        segments[i].ornekSayisi = 0;
        while (f.available()) {
          f.readStringUntil('\n');
          segments[i].ornekSayisi++;
        }

        f.close();
        segments[i].var = true;

        Serial.print(F("   Segment "));
        Serial.print(i + 1);
        Serial.print(F(": "));
        Serial.print(segments[i].malzemeAdi);
        Serial.print(F(" ("));
        Serial.print(segments[i].ornekSayisi);
        Serial.println(F(" örnek)"));
      }
    }
  }
}

void sdSegmentKaydet(uint8_t segmentNo, const char* malzemeAdi) {
  if (segmentNo >= MAX_SEGMENTS) return;

  Serial.println(F("\n╔════════════════════════════════════════════════╗"));
  Serial.print(F("║  SEGMENT "));
  Serial.print(segmentNo + 1);
  Serial.println(F(" KAYIT BAŞLIYOR                    ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));

  // Dosya oluştur
  File f = SD.open(segments[segmentNo].dosyaAdi, FILE_WRITE);
  if (!f) {
    Serial.println(F("✗ Dosya oluşturulamadı!"));
    return;
  }

  // İlk satır: Malzeme adı
  f.println(malzemeAdi);

  // Başlık satırı
  f.println(F("PULSE,ENC_BIG,A0"));

  f.close();

  strcpy(segments[segmentNo].malzemeAdi, malzemeAdi);
  segments[segmentNo].var = true;

  Serial.println(F("✓ Segment dosyası hazır."));
  Serial.println(F("✓ Kayıt başlatılabilir.\n"));
}

// ═══════════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════════
void setup() {
  // ─────────────────────────────────────────────────────────────
  // SERİ HABERLEŞME
  // ─────────────────────────────────────────────────────────────
  Serial.begin(115200);
  DELTA_SERIAL.begin(DELTA_BAUD);
  delay(500);

  Serial.println(F("\n\n╔════════════════════════════════════════════════╗"));
  Serial.println(F("║      TAM SİSTEM TEST MODU v1.0                ║"));
  Serial.println(F("║      Çoklu Segment + Delta + Röle             ║"));
  Serial.println(F("╚════════════════════════════════════════════════╝\n"));

  // ─────────────────────────────────────────────────────────────
  // ENCODER'LAR
  // ─────────────────────────────────────────────────────────────
  Serial.println(F(">> Encoder'lar başlatılıyor..."));
  encZ.begin();
  encX.begin();
  encB.begin();
  Serial.println(F("   ✓ Encoder'lar hazır"));

  // ─────────────────────────────────────────────────────────────
  // MOTOR PİNLERİ
  // ─────────────────────────────────────────────────────────────
  Serial.println(F(">> Motor pinleri ayarlanıyor..."));
  pinMode(ENA1_PIN, OUTPUT);
  pinMode(ENA2_PIN, OUTPUT);
  pinMode(ENA3_PIN, OUTPUT);
  digitalWrite(ENA1_PIN, LOW);
  digitalWrite(ENA2_PIN, LOW);
  digitalWrite(ENA3_PIN, LOW);
  Serial.println(F("   ✓ Motorlar aktif"));

  // ─────────────────────────────────────────────────────────────
  // LİMİT SWITCH VE EMERGENCY STOP
  // ─────────────────────────────────────────────────────────────
  Serial.println(F(">> Limit switch ve emergency stop..."));
  pinMode(LIMIT_Z_PIN, INPUT_PULLUP);
  pinMode(LIMIT_X_PIN, INPUT_PULLUP);
  pinMode(LIMIT_B_PIN, INPUT_PULLUP);
  pinMode(EMERGENCY_STOP_PIN, INPUT_PULLUP);
  Serial.println(F("   ✓ Güvenlik sistemleri hazır"));

  // ─────────────────────────────────────────────────────────────
  // RÖLE PİNİ
  // ─────────────────────────────────────────────────────────────
  Serial.println(F(">> Röle pini ayarlanıyor..."));
  pinMode(ROLE_PIN, OUTPUT);
  digitalWrite(ROLE_PIN, LOW);  // Kapalı başlat
  Serial.print(F("   ✓ Röle pin: "));
  Serial.println(ROLE_PIN);

  // ─────────────────────────────────────────────────────────────
  // SD KART
  // ─────────────────────────────────────────────────────────────
  if (sdBaslat()) {
    sdSegmentleriYukle();
  } else {
    Serial.println(F("⚠️  SD kart yok, segment kayıt/oynatma devre dışı!"));
  }

  // ─────────────────────────────────────────────────────────────
  // DELTA EKRANA İLK DURUM
  // ─────────────────────────────────────────────────────────────
  deltaGonderDurum();

  Serial.println(F("\n>> Sistem hazır! 'HELP' yazın.\n"));
}

// ═══════════════════════════════════════════════════════════════
// LOOP
// ═══════════════════════════════════════════════════════════════
void loop() {
  // ─────────────────────────────────────────────────────────────
  // SERİ KOMUT OKUMA (Debug için)
  // ─────────────────────────────────────────────────────────────
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (komutBuffer.length() > 0) {
        yeniKomutVar = true;
      }
    } else {
      komutBuffer += c;
    }
  }

  // ─────────────────────────────────────────────────────────────
  // DELTA EKRAN KOMUT OKUMA
  // ─────────────────────────────────────────────────────────────
  deltaOkuKomut();

  // ─────────────────────────────────────────────────────────────
  // KOMUT İŞLEME
  // ─────────────────────────────────────────────────────────────
  if (yeniKomutVar) {
    Serial.print(F("\n[KOMUT] \""));
    Serial.print(komutBuffer);
    Serial.println(F("\""));

    komutIsle(komutBuffer);

    komutBuffer = "";
    yeniKomutVar = false;
    toplamKomut++;

    if (sistemModu == MODE_IDLE) {
      Serial.println(F("\n>> Yeni komut bekleniyor...\n"));
    }
  }

  // ─────────────────────────────────────────────────────────────
  // GÜVENLIK KONTROLLERİ
  // ─────────────────────────────────────────────────────────────
  pulseAtLimitKontrolEkle();

  static bool oncekiEmergency = false;
  bool suankiEmergency = pulseAtEmergencyStop();

  if (suankiEmergency && !oncekiEmergency) {
    Serial.println(F("\n\n╔════════════════════════════════════════════════╗"));
    Serial.println(F("║     ⚠️  EMERGENCY STOP BASILDI! ⚠️            ║"));
    Serial.println(F("╚════════════════════════════════════════════════╝\n"));

    // RÖLE'Yİ HEMEN KAPAT!
    if (roleAktif) {
      roleKapat();
    }

    sistemModu = MODE_IDLE;
    deltaGonderDurum();
  } else if (!suankiEmergency && oncekiEmergency) {
    Serial.println(F("\n>> Emergency stop bırakıldı.\n"));
  }

  oncekiEmergency = suankiEmergency;

  // ─────────────────────────────────────────────────────────────
  // MOTOR ARKA PLAN İŞLEYİCİSİ
  // ─────────────────────────────────────────────────────────────
  if (sistemModu == MODE_IDLE) {
    useMotor(MOTOR_Z);
    pulseAt(0, 0, 0);
    useMotor(MOTOR_X);
    pulseAt(0, 0, 0);
    useMotor(MOTOR_B);
    pulseAt(0, 0, 0);
  }

  // ─────────────────────────────────────────────────────────────
  // DELTA EKRANA PERİYODİK DURUM GÖNDER (5 saniyede bir)
  // ─────────────────────────────────────────────────────────────
  static unsigned long lastDeltaUpdate = 0;
  if (millis() - lastDeltaUpdate > 5000) {
    deltaGonderDurum();
    lastDeltaUpdate = millis();
  }
}

// ═══════════════════════════════════════════════════════════════
// KOMUT İŞLEME FONKSİYONU
// ═══════════════════════════════════════════════════════════════
void komutIsle(String cmd);  // Forward declaration
void komutYardim();
void komutSegmentList();
void komutSegmentSelect(uint8_t segNo);
void komutSegmentKayit();
void komutSegmentOynat(uint8_t segNo);
void komutRoleTest();

void komutIsle(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  Serial.println(F("─────────────────────────────────────────────"));

  if (cmd == "HELP" || cmd == "?") {
    komutYardim();
  }
  else if (cmd == "ENCODER" || cmd == "ENC") {
    Serial.println(F("✓ ENCODER DEĞERLERİ:"));
    Serial.print(F("  Z: ")); Serial.println(encZ.getPosition());
    Serial.print(F("  X: ")); Serial.println(encX.getPosition());
    Serial.print(F("  B: ")); Serial.println(encB.getPosition());
  }
  else if (cmd == "STOP") {
    Serial.println(F("✓ TÜM MOTORLAR DURDURULUYOR!"));
    pulseAtHepsiniDurdur();
    if (roleAktif) roleKapat();
    sistemModu = MODE_IDLE;
    deltaGonderDurum();
  }
  else if (cmd == "HOME") {
    Serial.println(F("✓ HOMING BAŞLATIYOR..."));
    sistemModu = MODE_HOMING;
    deltaGonderDurum();

    bool sonuc = homeAll();

    sistemModu = MODE_IDLE;
    deltaGonderDurum();

    if (sonuc) {
      Serial.println(F("\n✓ Homing tamamlandı."));
    }
  }
  else if (cmd == "SEG_LIST") {
    komutSegmentList();
  }
  else if (cmd.startsWith("SEG_SELECT ")) {
    uint8_t seg = cmd.substring(11).toInt();
    komutSegmentSelect(seg);
  }
  else if (cmd == "SEG_KAYIT") {
    komutSegmentKayit();
  }
  else if (cmd.startsWith("SEG_OYNAT ")) {
    uint8_t seg = cmd.substring(10).toInt();
    komutSegmentOynat(seg);
  }
  else if (cmd == "ROLE_ON") {
    roleAc();
  }
  else if (cmd == "ROLE_OFF") {
    roleKapat();
  }
  else if (cmd == "ROLE_TEST") {
    komutRoleTest();
  }
  else if (cmd == "DELTA_STATUS") {
    deltaGonderDurum();
    Serial.println(F("✓ Durum Delta'ya gönderildi."));
  }
  else {
    Serial.println(F("✗ Bilinmeyen komut! 'HELP' yazın."));
  }

  Serial.println(F("─────────────────────────────────────────────"));
}

void komutYardim() {
  Serial.println(F("✓ TEST SİSTEMİ KOMUT LİSTESİ:\n"));

  Serial.println(F("  [Temel Komutlar]"));
  Serial.println(F("    HELP            : Bu yardım menüsü"));
  Serial.println(F("    ENCODER         : Encoder değerleri"));
  Serial.println(F("    STOP            : Tüm motorları durdur"));
  Serial.println(F("    HOME            : Homing yap"));

  Serial.println(F("\n  [Segment İşlemleri]"));
  Serial.println(F("    SEG_LIST        : Tüm segmentleri listele"));
  Serial.println(F("    SEG_SELECT <n>  : Segment seç (1-5)"));
  Serial.println(F("    SEG_KAYIT       : Seçili segmente kayıt"));
  Serial.println(F("    SEG_OYNAT <n>   : Segment oynat"));

  Serial.println(F("\n  [Röle Kontrol]"));
  Serial.println(F("    ROLE_ON         : Röle aç"));
  Serial.println(F("    ROLE_OFF        : Röle kapat"));
  Serial.println(F("    ROLE_TEST       : Röle test (yanıp söner)"));

  Serial.println(F("\n  [Delta Ekran]"));
  Serial.println(F("    DELTA_STATUS    : Durum bilgisi gönder"));

  Serial.println(F("\n  [Örnekler]"));
  Serial.println(F("    SEG_SELECT 1    → Segment 1'i seç"));
  Serial.println(F("    SEG_KAYIT       → Kayıt başlat"));
  Serial.println(F("    SEG_OYNAT 1     → Segment 1'i oynat"));
}

void komutSegmentList() {
  Serial.println(F("✓ KAYITLI SEGMENTLER:\n"));
  Serial.println(F(" NO |  MALZEME ADI        | ÖRNEK | DURUM"));
  Serial.println(F("────┼─────────────────────┼───────┼───────"));

  for (uint8_t i = 0; i < MAX_SEGMENTS; i++) {
    Serial.print(F("  "));
    Serial.print(i + 1);
    Serial.print(F(" | "));

    if (segments[i].var) {
      Serial.print(segments[i].malzemeAdi);
      for (int j = strlen(segments[i].malzemeAdi); j < 19; j++) {
        Serial.print(F(" "));
      }
      Serial.print(F(" | "));
      Serial.print(segments[i].ornekSayisi);
      Serial.print(F("     | ✓"));
    } else {
      Serial.print(F("(boş)              | -     | ✗"));
    }

    Serial.println();
  }

  Serial.println();
}

void komutSegmentSelect(uint8_t segNo) {
  if (segNo < 1 || segNo > MAX_SEGMENTS) {
    Serial.println(F("✗ Geçersiz segment! (1-5 arası)"));
    return;
  }

  aktifSegment = segNo - 1;
  Serial.print(F("✓ Segment "));
  Serial.print(segNo);
  Serial.println(F(" seçildi."));

  if (!segments[aktifSegment].var) {
    Serial.println(F("⚠️  Bu segment boş. SEG_KAYIT ile kayıt yapın."));
  } else {
    Serial.print(F("   Malzeme: "));
    Serial.println(segments[aktifSegment].malzemeAdi);
    Serial.print(F("   Örnek sayısı: "));
    Serial.println(segments[aktifSegment].ornekSayisi);
  }
}

void komutSegmentKayit() {
  Serial.println(F("✓ SEGMENT KAYIT MODUına GİRİYOR..."));
  Serial.println(F("⚠️  ŞU AN SADECE TEST MODU - GERÇEK KAYIT YOK"));
  Serial.println(F("   Gerçek kayıt için KayitModulu entegre edilecek."));

  // TODO: Gerçek kayıt modülü entegrasyonu
  // sdSegmentKaydet(aktifSegment, "Test Malzeme");
  // kayitBaslatBlocking();
  // SD'ye yaz
}

void komutSegmentOynat(uint8_t segNo) {
  if (segNo < 1 || segNo > MAX_SEGMENTS) {
    Serial.println(F("✗ Geçersiz segment!"));
    return;
  }

  if (!segments[segNo - 1].var) {
    Serial.println(F("✗ Bu segment boş!"));
    return;
  }

  Serial.println(F("✓ SEGMENT OYNATMA BAŞLIYOR..."));
  Serial.println(F("⚠️  ŞU AN SADECE TEST MODU - GERÇEK OYNATMA YOK"));

  // TODO: Gerçek oynatma modülü
  // 1. SD'den oku
  // 2. Hizalama yap
  // 3. Röle aç
  // 4. Oynat
  // 5. Bitince röle kapat
}

void komutRoleTest() {
  Serial.println(F("✓ RÖLE TEST MODU (3 kez yanıp söner)\n"));

  for (int i = 0; i < 3; i++) {
    Serial.print(F("   ["));
    Serial.print(i + 1);
    Serial.println(F("] Röle AÇIK"));
    roleAc();
    delay(1000);

    Serial.print(F("   ["));
    Serial.print(i + 1);
    Serial.println(F("] Röle KAPALI"));
    roleKapat();
    delay(1000);
  }

  Serial.println(F("\n✓ Röle testi tamamlandı."));
}
