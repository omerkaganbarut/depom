# 🎯 TEST SİSTEMİ KULLANIM REHBERİ

## 📚 BÖLÜM 1: SİSTEM YAPISI

### İKİ FARKLI MOD VAR:

```
┌─────────────────────────────────────────────────┐
│           TEST MODU (Şu an bu moddayız)        │
├─────────────────────────────────────────────────┤
│ Dosya: test/test_full_system.cpp               │
│ Kontrol: Seri komutlar (HELP, SEG_KAYIT, vb.)  │
│ Debug: Bol bol mesaj                            │
│ Amaç: Her şeyi test et, sorun bul               │
└─────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────┐
│         GERÇEK MOD (Üretim için)                │
├─────────────────────────────────────────────────┤
│ Dosya: src/main.cpp                             │
│ Kontrol: Delta ekran                            │
│ Debug: Minimal                                  │
│ Amaç: Makinede çalış, üretim yap                │
└─────────────────────────────────────────────────┘
```

---

## 🚀 BÖLÜM 2: TEST MODUNU ÇALIŞTIRMA

### Adım 1: Test Modunu Derle ve Yükle

```bash
# VSCode'da Terminal aç (Ctrl+`)

# Test modunu derle
pio run -e test_full_system

# Arduino'ya yükle
pio run -e test_full_system -t upload

# Seri monitör aç
pio device monitor -e test_full_system
```

### Adım 2: İlk Çalışma Ekranı

```
╔════════════════════════════════════════════════╗
║      TAM SİSTEM TEST MODU v1.0                ║
║      Çoklu Segment + Delta + Röle             ║
╚════════════════════════════════════════════════╝

>> Encoder'lar başlatılıyor...
   ✓ Encoder'lar hazır
>> Motor pinleri ayarlanıyor...
   ✓ Motorlar aktif
>> Limit switch ve emergency stop...
   ✓ Güvenlik sistemleri hazır
>> Röle pini ayarlanıyor...
   ✓ Röle pin: 45
>> SD kart başlatılıyor... ✓
>> SD karttan segmentler yükleniyor...

>> Sistem hazır! 'HELP' yazın.
```

---

## 📋 BÖLÜM 3: TEMEL KOMUTLAR

### HELP - Komut Listesi

```
HELP

✓ TEST SİSTEMİ KOMUT LİSTESİ:

  [Temel Komutlar]
    HELP            : Bu yardım menüsü
    ENCODER         : Encoder değerleri
    STOP            : Tüm motorları durdur
    HOME            : Homing yap

  [Segment İşlemleri]
    SEG_LIST        : Tüm segmentleri listele
    SEG_SELECT <n>  : Segment seç (1-5)
    SEG_KAYIT       : Seçili segmente kayıt
    SEG_OYNAT <n>   : Segment oynat

  [Röle Kontrol]
    ROLE_ON         : Röle aç
    ROLE_OFF        : Röle kapat
    ROLE_TEST       : Röle test (yanıp söner)

  [Delta Ekran]
    DELTA_STATUS    : Durum bilgisi gönder
```

### ENCODER - Encoder Değerleri

```
ENCODER

✓ ENCODER DEĞERLERİ:
  Z: 0
  X: 0
  B: 0
```

---

## 🏠 BÖLÜM 4: HOMING SİSTEMİ

### HOME - Tüm Motorları Homing Yap

```
HOME

✓ HOMING BAŞLATIYOR...

[1/3] Motor Z homing...
══════════════════════════════════════
>> FAZ 1: Hızlı git...
   ✓ Limit switch basıldı!
>> FAZ 2: Yavaşça geri gel...
   ✓ Limit switch bırakıldı.
>> FAZ 3: Çok yavaş tekrar yaklaş...
   ✓ Hassas konum bulundu!
╔════════════════════════════════════╗
║  Motor Z HOMING TAMAMLANDI! ✓      ║
╚════════════════════════════════════╝

... (Motor X ve Motor BIG aynı şekilde)

╔════════════════════════════════════╗
║ TÜM MOTORLAR HOMING TAMAMLANDI! ✓  ║
╚════════════════════════════════════╝

✓ Homing tamamlandı.
```

---

## 🗂️ BÖLÜM 5: SEGMENT YÖNETİMİ

### SEG_LIST - Segmentleri Listele

```
SEG_LIST

✓ KAYITLI SEGMENTLER:

 NO |  MALZEME ADI        | ÖRNEK | DURUM
────┼─────────────────────┼───────┼───────
  1 | (boş)               | -     | ✗
  2 | (boş)               | -     | ✗
  3 | (boş)               | -     | ✗
  4 | (boş)               | -     | ✗
  5 | (boş)               | -     | ✗
```

### SEG_SELECT - Segment Seç

```
SEG_SELECT 1

✓ Segment 1 seçildi.
⚠️  Bu segment boş. SEG_KAYIT ile kayıt yapın.
```

### SEG_KAYIT - Kayıt Yap (Şu an Test Modu)

```
SEG_KAYIT

✓ SEGMENT KAYIT MODUNA GİRİYOR...
⚠️  ŞU AN SADECE TEST MODU - GERÇEK KAYIT YOK
   Gerçek kayıt için KayitModulu entegre edilecek.
```

**NOT:** Gerçek kayıt için aşağıdaki entegrasyon yapılacak:

```cpp
void komutSegmentKayit() {
  // 1. SD dosyası oluştur
  sdSegmentKaydet(aktifSegment, "Alüminyum");

  // 2. Kayıt başlat (KayitModulu.h)
  sistemModu = MODE_KAYIT_SEGMENT;
  kayitBaslatBlocking();

  // 3. Veriler SD'ye yazıldı
  segments[aktifSegment].var = true;
  segments[aktifSegment].ornekSayisi = kayitIndex();

  sistemModu = MODE_IDLE;
  Serial.println("✓ Segment kayıt tamamlandı!");
}
```

### SEG_OYNAT - Segmenti Oynat (Şu an Test Modu)

```
SEG_OYNAT 1

✓ SEGMENT OYNATMA BAŞLIYOR...
⚠️  ŞU AN SADECE TEST MODU - GERÇEK OYNATMA YOK
```

**NOT:** Gerçek oynatma için aşağıdaki entegrasyon yapılacak:

```cpp
void komutSegmentOynat(uint8_t segNo) {
  // 1. SD'den segment verilerini oku
  File f = SD.open(segments[segNo-1].dosyaAdi, FILE_READ);

  // 2. Verileri yükle (samples[] dizisine)
  // ... (parse et)

  // 3. Hizalama yap
  oynatmaBaslat();

  // 4. Hizalama bittikten sonra RÖLE AÇ!
  roleAc();

  // 5. Oynatma döngüsü
  sistemModu = MODE_OYNATMA_SEGMENT;
  while (!oynatmaTamamlandi()) {
    oynatmaLoop();
    pulseAtLimitKontrolEkle();  // Güvenlik
  }

  // 6. Bitince RÖLE KAPAT!
  roleKapat();

  sistemModu = MODE_IDLE;
  Serial.println("✓ Oynatma tamamlandı!");
}
```

---

## 🔥 BÖLÜM 6: RÖLE KONTROLÜ

### ROLE_TEST - Röle Testi

```
ROLE_TEST

✓ RÖLE TEST MODU (3 kez yanıp söner)

   [1] Röle AÇIK
🔥 RÖLE AÇIK (Kaynak başladı)
   [1] Röle KAPALI
❄️  RÖLE KAPALI (Kaynak durdu)

   [2] Röle AÇIK
🔥 RÖLE AÇIK (Kaynak başladı)
   [2] Röle KAPALI
❄️  RÖLE KAPALI (Kaynak durdu)

   [3] Röle AÇIK
🔥 RÖLE AÇIK (Kaynak başladı)
   [3] Röle KAPALI
❄️  RÖLE KAPALI (Kaynak durdu)

✓ Röle testi tamamlandı.
```

### ROLE_ON / ROLE_OFF - Manuel Röle Kontrolü

```
ROLE_ON
🔥 RÖLE AÇIK (Kaynak başladı)

ROLE_OFF
❄️  RÖLE KAPALI (Kaynak durdu)
```

**RÖLE BAGLANTISI:**
```
Arduino Pin 45 → Röle IN
Röle COM → Kaynak makinesi AC
Röle NO  → Kaynak makinesi 220V
```

---

## 📡 BÖLÜM 7: DELTA EKRAN HABERLEŞMESİ

### Delta Ekran Bağlantısı

```
Arduino Mega Serial1:
  TX1 (Pin 18) → Delta Ekran RX
  RX1 (Pin 19) → Delta Ekran TX
  GND → Delta Ekran GND
```

### DELTA_STATUS - Durum Gönder

```
DELTA_STATUS

✓ Durum Delta'ya gönderildi.
```

**Delta'ya gönderilen mesaj:**
```
STATUS:IDLE:0:0:0:0
```

Format: `STATUS:<MOD>:<SEGMENT>:<PROGRESS>:<ENC_B>:<ENC_X>`

### Delta'dan Komut Alma

**Otomatik çalışır, her 5 saniyede durum güncellenir**

Delta ekran gönderirse:
```
CMD:START_KAYIT:1\n
```

Arduino yanıtı:
```
[DELTA] "CMD:START_KAYIT:1"
>> Delta: Segment 1 kayıt başlat
```

---

## ⚠️ BÖLÜM 8: EMERGENCY STOP

### Emergency Stop Testi

1. Motorları çalıştır:
```
HOME
```

2. Emergency stop butonuna bas

```
╔════════════════════════════════════════════════╗
║     ⚠️  EMERGENCY STOP BASILDI! ⚠️            ║
╚════════════════════════════════════════════════╝

❄️  RÖLE KAPALI (Kaynak durdu)

>> Emergency stop bırakıldı.
```

**NE OLUR?**
- ✅ Tüm motorlar anında durur
- ✅ Röle anında kapanır (kaynak durur)
- ✅ Sistem IDLE moduna döner
- ✅ Delta ekrana bildirim gider

---

## 🧪 BÖLÜM 9: TAM SİSTEM TEST SENARYOSU

### Test Adımları (Sırayla)

```bash
# 1. Encoder kontrolü
ENCODER
# Sonuç: Z=0, X=0, B=0

# 2. Homing yap
HOME
# Sonuç: Tüm motorlar limit switch'e gitti, hassas konum bulundu

# 3. Encoder tekrar kontrol
ENCODER
# Sonuç: Z=0, X=0, B=0 (home pozisyonu)

# 4. Segment listele
SEG_LIST
# Sonuç: Hepsi boş

# 5. Segment 1'i seç
SEG_SELECT 1
# Sonuç: Segment 1 seçildi (boş)

# 6. Röle test
ROLE_TEST
# Sonuç: 3 kez yanıp söndü

# 7. Delta ekrana durum gönder
DELTA_STATUS
# Sonuç: STATUS:IDLE:0:0:0:0 gönderildi

# 8. Emergency stop test
# [Butona bas]
# Sonuç: Sistem durduruldu, röle kapandı

# 9. Stop komutu
STOP
# Sonuç: Tüm motorlar durdu

# 10. Segment kayıt (şu an test modu)
SEG_KAYIT
# Sonuç: Test mesajı gösterildi

# 11. Segment oynat (şu an test modu)
SEG_OYNAT 1
# Sonuç: Test mesajı gösterildi
```

---

## 🔧 BÖLÜM 10: GERÇEK MODA GEÇİŞ

### Gerçek Mod Nasıl Çalışır?

```bash
# Gerçek modu derle ve yükle
pio run -e mega2560 -t upload

# Seri monitör aç (sadece hata mesajları için)
pio device monitor -e mega2560
```

**Gerçek modda:**
- Seri komutlar yok (Delta ekran kontrol eder)
- Minimal log
- Üretim için optimize edilmiş

---

## 📊 BÖLÜM 11: SD KART YAPISI

### SD Kart Klasör Yapısı

```
SD:/
├── segments/
│   ├── seg_1.txt      (Alüminyum)
│   ├── seg_2.txt      (Çelik)
│   ├── seg_3.txt      (Bakır)
│   ├── seg_4.txt      (Plastik)
│   └── seg_5.txt      (Ahşap)
└── logs/
    └── error_log.txt  (Hata kayıtları)
```

### Segment Dosya Formatı (seg_1.txt)

```
Alüminyum
PULSE,ENC_BIG,A0
0,0,512
100,100,550
200,200,600
300,300,650
...
4000,4000,900
```

---

## 🎯 BÖLÜM 12: SIRA SENDE!

### Şimdi Ne Yapmalısın?

1. **Test modunu yükle:**
```bash
pio run -e test_full_system -t upload
```

2. **Seri monitör aç:**
```bash
pio device monitor
```

3. **İlk komutu yaz:**
```
HELP
```

4. **Encoder'ları kontrol et:**
```
ENCODER
```

5. **Homing yap:**
```
HOME
```

6. **Röle test et:**
```
ROLE_TEST
```

7. **Segmentleri incele:**
```
SEG_LIST
```

8. **Delta durum gönder:**
```
DELTA_STATUS
```

9. **Emergency stop'u test et:**
- Butona bas
- Sonucu gözle

10. **Geri bildirim ver:**
- Hangi özellik çalıştı?
- Hangi özellik çalışmadı?
- Ne eklemek istersin?

---

## 🐛 BÖLÜM 13: SORUN GİDERME

### Sorun: SD kart başlatılamadı

**Çözüm:**
- SD kart takılı mı kontrol et
- SD_CS_PIN doğru mu? (Pin 53)
- SD kart formatlanmış mı? (FAT16/FAT32)

### Sorun: Limit switch çalışmıyor

**Çözüm:**
- Pull-up direnç var mı?
- Pin numarası doğru mu?
- Mekanik bağlantı sağlam mı?

### Sorun: Röle çalışmıyor

**Çözüm:**
- Pin 45 doğru mu?
- Röle 5V'a bağlı mı?
- LED ile test et (pinMode(45, OUTPUT); digitalWrite(45, HIGH);)

### Sorun: Delta ekran yanıt vermiyor

**Çözüm:**
- Serial1 (Pin 18-19) bağlantısı doğru mu?
- Baud rate 9600 mu?
- GND ortak mı?

---

## ✅ BÖLÜM 14: YAPILACAKLAR

- [ ] Gerçek kayıt modülü entegrasyonu (SEG_KAYIT)
- [ ] Gerçek oynatma modülü entegrasyonu (SEG_OYNAT)
- [ ] SD karttan segment okuma
- [ ] Segment silme (SEG_DELETE)
- [ ] Delta ekran gerçek haberleşme testi
- [ ] Röle gerçek kaynak makinesi bağlantısı
- [ ] A0 filtre entegrasyonu
- [ ] İlerleme yüzdesi hesaplama
- [ ] Hata log sistemi
- [ ] Factory reset özelliği

---

## 🎉 BAŞARILAR!

Test sistemin hazır! Şimdi her şeyi adım adım test edebilir, sorunları bulabilir ve gerçek sisteme geçmeden önce her şeyin çalıştığından emin olabilirsin.

**Sorular?**
- HELP yazarak komutları gör
- TEST_PLAN.md'yi oku
- Delta ekran protokolünü incele
- Kod içindeki TODO'ları takip et
