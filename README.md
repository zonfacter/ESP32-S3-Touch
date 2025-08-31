# ESP32-S3-Touch LCD 2.8" - Basisprojekt

Modulare Basis für das **Waveshare ESP32-S3-Touch-LCD-2.8** mit ST7789T3 Display, CST328 Touch, QMI8658C IMU, I2S-Audio/APA2026 und optionalem RS485-Modul.

## 🎯 Projektziele

- **Display (ST7789T3)** zuverlässig initialisieren (korrekte Farben, Backlight)
- **Touch (CST328)** robust lesen (IRQ + Fallback), Mapping & Tracking, stabile Gesten
- **Audio (APA2026 via I2S)** als non-blocking Feedback für Gesten  
- **IMU (QMI8658C)** auslesen und im HUD anzeigen
- **RS485 (UART1 + DE/RE)** als optionales Kommunikationsmodul
- **Modularer Code** (mehrere Dateien/Ordner), Arduino IDE 2.3.6 kompatibel

## 🔧 Hardware & Pinmapping

### Display ST7789T3 (SPI3 / LovyanGFX)
```
SCLK=40, MOSI=45, MISO=-1, DC=41, CS=42, RST=39
Backlight (BL) GPIO 5 (active HIGH)
```
**LovyanGFX Config:** `spi_3wire = false` (4-Wire mit DC), `invert = true`, `rgb_order = false`

### Touch CST328 (I2C1 / Wire1)  
```
SDA=1, SCL=3, INT=4, RST=2, Addr=0x1A, Freq=400kHz
INT: INPUT_PULLUP, FALLING (idle HIGH), nach Reset ca. 200ms warten
```

### IMU QMI8658C (I2C0 / Wire)
```
SDA=11, SCL=10, INT1=13, INT2=12, Addr=0x6B
```

### Audio / I2S (APA2026)
```
LRCK=38, BCK=48, DIN=47
```

### RS485 (optional)
```
UART1 TX=15, RX=14, DE/RE=16 (HIGH → TX)
```

### I²C-Adressen (Onboard)
- **I2C0:** `0x51`, `0x6B`, `0x7E`
- **I2C1:** `0x1A` (Touch)

## 📁 Projektstruktur

```
src/
├── app/            # App.h/.cpp (Main-Loop, Init, HUD)
├── display/        # DisplayManager (LovyanGFX ST7789T3)
├── touch/          # CST328Touch (I2C, IRQ, Mapping/Tracking)
├── gestures/       # GestureEngine (State-Machine; Events einmalig)
├── audio/          # AudioI2S (I2S, non-blocking Töne, Flood-Guard)
├── imu/            # QMI8658 (I2C-Init/Burst-Read)
├── comm/           # RS485Bus + SerialConsole
└── config/         # pins.h, params.h (Konstanten/Schwellen)
```

## 🚀 Build-Konfiguration

- **Arduino IDE:** 2.3.6
- **Board:** "ESP32S3 Dev Module"  
- **PSRAM:** Enabled
- **Partition:** "Huge APP"
- **Libraries:** LovyanGFX

**Resource Usage:** Flash ~13%, RAM ~7%

## ✅ Erreichte Meilensteine  

- ✅ **Modularer Code** mit klaren Modulen/Includes; kompiliert sauber
- ✅ **Display** sichtbar (Backlight-Pin 5; korrekte Farben; 4-Wire-SPI)
- ✅ **I²C-Scan** OK: 0x51/0x6B/0x7E sowie Touch 0x1A erkannt
- ✅ **Audio** I2S-Töne funktionieren, non-blocking (10ms Timeout) + Flood-Guard  
- ✅ **RS485-Modul** integriert mit Console-Commands

## ⚠️ Aktuelle Probleme

- 🔧 **Touch-Release-Erkennung:** `readFrame()` returniert niemals `false`
- 🔧 **Gesture-Blocking:** Nach erster Geste bleibt System in aktivem Zustand
- 🔧 **IRQ-Handling:** Touch-Interrupt wird möglicherweise nicht korrekt gecleared

## 🎛️ Wichtige Parameter

### Display & Touch Mapping
```cpp
// Display
static constexpr uint16_t DISPLAY_WIDTH  = 320;
static constexpr uint16_t DISPLAY_HEIGHT = 240;  
static constexpr uint8_t  DISPLAY_ROTATION = 1;

// Touch Raw → Display Mapping
static constexpr int  TOUCH_RAW_X_MIN = 0, TOUCH_RAW_X_MAX = 240;
static constexpr int  TOUCH_RAW_Y_MIN = 0, TOUCH_RAW_Y_MAX = 320;
static constexpr bool TOUCH_SWAP_XY  = true;
static constexpr bool TOUCH_INVERT_X = false;
static constexpr bool TOUCH_INVERT_Y = true;
```

### Gesture Thresholds
```cpp
static constexpr uint8_t  MAX_TOUCH_POINTS    = 5;
static constexpr uint16_t TAP_MAX_DURATION    = 250; // ms
static constexpr uint16_t TAP_MAX_MOVEMENT    = 20;  // px
static constexpr uint16_t DOUBLE_TAP_INTERVAL = 400; // ms
static constexpr uint16_t LONG_PRESS_DURATION = 800; // ms
static constexpr uint16_t SWIPE_MIN_DISTANCE  = 30;  // px
static constexpr float    SWIPE_AXIS_RATIO    = 1.5f;

// Touch Filtering
static constexpr uint16_t TOUCH_MIN_STRENGTH  = 20;
static constexpr uint16_t TOUCH_SETTLE_MS     = 10;
```

## 🧪 Quick-Test

1. **Flash & Serial Monitor** (115200 baud)
2. **I²C-Scan** prüfen (0x51/0x6B/0x7E, 0x1A)  
3. **Display** zeigt HUD (FPS/IMU)
4. **Touch-Gesten:** 
   - Tap → kurzer Ton
   - DoubleTap → doppelt  
   - LongPress (≥800ms)
   - Swipe (≥30px klar achsig)
   - Pinch/Rotate
5. **RS485 (optional):** `rs485send hello`, `rs485baud 9600`, `rs485echo on`

## 🔑 Known-Good Fixes

- **Backlight:** GPIO 5 (HIGH = an)
- **LovyanGFX:** 4-Wire (`spi_3wire=false`), `invert=true`, `rgb_order=false`  
- **CST328:** I2C1 (SDA=1/SCL=3), INT=4 INPUT_PULLUP, IRQ FALLING
- **Audio:** `i2s_write` mit 10ms Timeout (keine Blocks), Flood-Guard

## 📋 Next Steps

- 🔧 **Touch-Release-Problem lösen** - CST328 Register-Clearing reparieren  
- 🎯 **TOUCH_MIN_STRENGTH** anhand realer Logs feinjustieren  
- 🎛️ **Gesture-Thresholds** mit Praxiswerten abgleichen  
- 🌐 **RS485-Feldtest** mit externen Geräten
- 🐞 **Debug-Logs** unter `#ifdef DEBUG_*` kapseln

## 📊 Status

**Basis steht und ist wartbar.** Display & Audio sind stabil. Touch-Hardware funktioniert, aber **Release-Erkennung benötigt kritischen Fix** für funktionsfähige Gestenerkennung.

---

**Hardware:** Waveshare ESP32-S3 Touch LCD 2.8" | **IDE:** Arduino 2.3.6 | **Framework:** ESP32 Arduino Core
