# 🧪 GENC3ELMAKINE - KOMPLE TEST SİSTEMİ

## 📁 PROJE YAPISI

```
genc3elmakine/
├── src/
│   └── main.cpp                    ← GERÇEK PROJE (Delta ekran + Üretim)
│
└── test/
    ├── TEST_PLAN.md               ← Bu dosya
    ├── test_main.cpp              ← Test ana programı (Seri komut)
    ├── test_delta_ekran.cpp       ← Delta ekran haberleşme testi
    ├── test_sd_multi_segment.cpp  ← Çoklu segment SD testi
    ├── test_role_kontrol.cpp      ← Röle kontrol testi
    └── test_full_system.cpp       ← TAM SİSTEM entegrasyon testi
```

## 🎯 TEST SİSTEMİ MİMARİSİ

### TEST MODU vs GERÇEK MOD

```
┌─────────────────────────────────────────────────────────┐
│                    TEST MODU                            │
├─────────────────────────────────────────────────────────┤
│ - Seri komut ile kontrol                                │
│ - Debug mesajları bol bol                               │
│ - Her şey adım adım test edilir                         │
│ - SD karta "test_segment_1.txt" şeklinde yazar          │
│ - Röle test modu (LED ile simülasyon)                   │
│ - Delta ekran simülasyonu (Serial1)                     │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                   GERÇEK MOD                            │
├─────────────────────────────────────────────────────────┤
│ - Delta ekran ile kontrol                               │
│ - Minimal log                                           │
│ - Üretim amaçlı                                         │
│ - SD karta "segment_malzeme_X.txt" yazar                │
│ - Gerçek röle kontrolü                                  │
│ - Delta ekran (Serial2 veya Serial3)                    │
└─────────────────────────────────────────────────────────┘
```

## 🔧 ÖZELLİKLER

### 1. DELTA EKRAN HABERLEŞMESİ
```
Arduino → Delta Ekran
- Durum bilgisi (hangi moddasın, hangi segment)
- Encoder değerleri
- İlerleme yüzdesi

Delta Ekran → Arduino
- Komutlar: START_KAYIT, START_OYNAT, STOP
- Segment seçimi: SEG_1, SEG_2, SEG_3
- Parametre değişiklikleri
```

### 2. ÇOKLU SEGMENT YÖNETİMİ
```
Segment 1: Alüminyum (a0Min=100, a0Max=500)
Segment 2: Çelik (a0Min=200, a0Max=800)
Segment 3: Bakır (a0Min=150, a0Max=600)

SD Kart Yapısı:
/segments/
  ├── segment_1.txt
  ├── segment_2.txt
  └── segment_3.txt
```

### 3. RÖLE KONTROL (Kaynak Makinesi)
```
- Kayıt sırasında: Röle KAPALI
- Oynatma başlangıcında: Röle KAPALI
- İlk hizalamadan sonra: Röle AÇIK (kaynak başlar)
- Oynatma bitince: Röle KAPALI
- Emergency stop: Röle ANİNDA KAPALI
```

### 4. SD KART YÖNETİMİ
```
- Otomatik dosya oluşturma (segment_1.txt, segment_2.txt)
- Buffer sistemi (100 satır buffer, sonra SD'ye yaz)
- Veri formatı: PULSE, ENC_BIG, A0, TIMESTAMP
```

## 📝 TEST AŞAMALARI

### AŞAMA 1: Temel Motor Testi
```
Test dosyası: test_motor_basic.cpp
Amaç: PulseAt ve encoder testi
Komutlar:
  - M1 1000 0 100
  - ENC
  - STOP
```

### AŞAMA 2: Limit Switch + Emergency Stop
```
Test dosyası: test_safety.cpp
Amaç: Güvenlik sistemleri
Test:
  1. Motor çalıştır
  2. Limit switch'e değdir
  3. Emergency stop bas
  4. Otomatik durma kontrolü
```

### AŞAMA 3: SD Kart Yazma/Okuma
```
Test dosyası: test_sd_basic.cpp
Amaç: SD karta kayıt ve okuma
Test:
  1. Yeni dosya oluştur
  2. 1000 satır yaz
  3. Oku ve doğrula
```

### AŞAMA 4: Çoklu Segment
```
Test dosyası: test_multi_segment.cpp
Amaç: 3 farklı malzeme kaydet ve oynat
Test:
  1. Segment 1 kaydet (Alüminyum)
  2. Segment 2 kaydet (Çelik)
  3. Segment 3 kaydet (Bakır)
  4. Segment 1'i oynat
  5. Segment 2'yi oynat
```

### AŞAMA 5: Röle Kontrol
```
Test dosyası: test_role.cpp
Amaç: Kaynak rölesini test et
Test:
  1. Röle pin tanımla (LED ile simüle)
  2. Oynatma başlat
  3. Hizalama bitince röle aç
  4. Oynatma bitince röle kapat
  5. Emergency stop → röle kapat
```

### AŞAMA 6: Delta Ekran Simülasyonu
```
Test dosyası: test_delta_serial.cpp
Amaç: Delta ekran haberleşmesi
Test:
  1. Serial1 ile PC'den komut gönder
  2. Arduino yanıt ver
  3. Durum bilgisi gönder
  4. Komut al ve işle
```

### AŞAMA 7: TAM SİSTEM ENTEGRASYONTesti
```
Test dosyası: test_full_system.cpp
Amaç: Her şey bir arada
Test:
  1. HOME
  2. Segment 1 seç
  3. Kayıt başlat (röle kapalı)
  4. SD'ye yaz
  5. Oynat (hizalamadan sonra röle aç)
  6. Bitince röle kapat
  7. Delta ekrana rapor gönder
```

## 🚀 TEST NASIL YAPILIR?

### platformio.ini Ayarı
```ini
[env:test]
platform = atmelavr
board = megaatmr2560
framework = arduino
build_src_filter =
    -<src/*>           ; Ana kodu hariç tut
    +<test/test_full_system.cpp>  ; Sadece bu testi al
```

### Test Çalıştırma
```bash
# Test modunu çalıştır
pio run -e test -t upload

# Gerçek modu çalıştır
pio run -e megaatmr2560 -t upload
```

## 📊 DELTA EKRAN PROTOKOLÜ

### Komut Formatı (Delta → Arduino)
```
CMD:<KOMUT>:<PARAM>\n

Örnekler:
CMD:START_KAYIT:1\n        → Segment 1'e kayıt başlat
CMD:START_OYNAT:2\n        → Segment 2'yi oynat
CMD:STOP:0\n               → Durdur
CMD:HOME:0\n               → Homing yap
CMD:SEG_SELECT:3\n         → Segment 3'ü seç
```

### Durum Formatı (Arduino → Delta)
```
STATUS:<MOD>:<SEGMENT>:<PROGRESS>:<ENC_B>:<ENC_X>\n

Örnekler:
STATUS:IDLE:0:0:0:0\n
STATUS:KAYIT:1:45:1250:500\n
STATUS:OYNATMA:2:78:3500:1200\n
STATUS:ERROR:0:0:0:0\n
```

## 🎨 TEST KOMUTLARI (Seri Monitör)

```
# Temel Komutlar
HELP                    → Tüm komutları göster
BILGI                   → Sistem durumu
ENCODER                 → Encoder değerleri

# Motor Komutları
M1 1000 0 100          → Motor Z test
M2 1000 0 100          → Motor X test
M3 1000 0 100          → Motor BIG test

# Homing
HOME                   → Tüm motorları homing yap
HOME1                  → Sadece Motor Z

# Segment İşlemleri
SEG_LIST               → Tüm segmentleri listele
SEG_SELECT 1           → Segment 1'i seç
SEG_KAYIT              → Seçili segmente kayıt yap
SEG_OYNAT 1            → Segment 1'i oynat
SEG_DELETE 2           → Segment 2'yi sil

# SD Kart
SD_INFO                → SD kart bilgisi
SD_FORMAT              → SD kartı formatla (dikkatli!)
SD_LIST                → Dosyaları listele

# Röle
ROLE_ON                → Röle aç (test için)
ROLE_OFF               → Röle kapat
ROLE_TEST              → Röle test modu (yanıp söner)

# Delta Ekran
DELTA_SEND "mesaj"     → Delta ekrana mesaj gönder
DELTA_STATUS           → Durum bilgisi gönder

# Sistem
RESET                  → Arduino'yu resetle
FACTORY_RESET          → Tüm ayarları sıfırla
```

## 🐛 DEBUG SEVİYELERİ

```cpp
#define DEBUG_LEVEL_NONE  0  // Hiç log yok
#define DEBUG_LEVEL_ERROR 1  // Sadece hatalar
#define DEBUG_LEVEL_INFO  2  // Bilgi mesajları
#define DEBUG_LEVEL_DEBUG 3  // Her şey

// Config.h'de:
#define DEBUG_LEVEL DEBUG_LEVEL_DEBUG  // Test modu
// veya
#define DEBUG_LEVEL DEBUG_LEVEL_ERROR  // Gerçek mod
```

## 📦 YAPILACAKLAR LİSTESİ

- [x] PulseAt kütüphanesi
- [x] Limit switch + Emergency stop
- [x] Homing modülü
- [x] Seri komut sistemi
- [ ] SD kart çoklu segment
- [ ] Delta ekran haberleşmesi
- [ ] Röle kontrol modülü
- [ ] Segment yönetim sistemi
- [ ] Tam sistem entegrasyon testi
- [ ] Gerçek Delta ekran entegrasyonu

## 🎯 SONRAKİ ADIMLAR

1. `test_full_system.cpp` dosyasını inceleyerek başla
2. Her özelliği tek tek test et
3. Sorun varsa debug mesajlarını oku
4. Her şey çalışınca gerçek moda geç
5. Delta ekranı bağla ve test et
