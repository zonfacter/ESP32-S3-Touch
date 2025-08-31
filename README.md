# ESP32-S3-Touch
ESP32-S3 Touch LCD 2.8 – Basisprojekt (Waveshare)

Dieses Repo baut eine saubere, modulare Basis für das Waveshare ESP32-S3-Touch-LCD-2.8 (ST7789T3 Display, CST328 Touch, QMI8658C IMU, I2S-Audio/APA2026) inkl. optionalem RS485-Modul. Ziel ist ein wartbares Grundgerüst mit stabiler Touch-/Gestenerkennung, funktionierendem Display/Audio und klarer Trennung der Module.

Ziel(e)

Display (ST7789T3) zuverlässig initialisieren (korrekte Farben, Backlight).

Touch (CST328) robust lesen (IRQ + Fallback), Mapping & Tracking, stabile Gesten.

Audio (APA2026 via I2S) als non-blocking Feedback für Gesten.

IMU (QMI8658C) auslesen und im HUD anzeigen.

RS485 (UART1 + DE/RE) als optionales Kommunikationsmodul mit Konsolenbefehlen.

Modularer Code (mehrere Dateien/Ordner), Arduino IDE 2.3.6 kompatibel.

Hardware/Pinmapping (Waveshare ESP32-S3 Touch LCD 2.8)

Display ST7789T3 (SPI3 / LovyanGFX)

SCLK=40, MOSI=45, MISO=-1, DC=41, CS=42, RST=39

Backlight (BL) GPIO 5 (active HIGH)

LovyanGFX: cfg.spi_3wire = false (4-Wire mit DC), cfg.invert = true, cfg.rgb_order = false

Touch CST328 (I2C1 / Wire1)

SDA=1, SCL=3, INT=4, RST=2, Addr=0x1A, Freq=400 kHz

INT: INPUT_PULLUP, FALLING (idle HIGH), nach Reset ca. 200 ms warten

IMU QMI8658C (I2C0 / Wire)

SDA=11, SCL=10, INT1=13, INT2=12, Addr=0x6B

Audio / I2S (APA2026)

LRCK=38, BCK=48, DIN=47

RS485 (optional)

UART1 TX=15, RX=14, DE/RE=16 (HIGH → TX)

Onboard I²C-Adressen (Scan)

0x51, 0x6B, 0x7E (Touch separat: 0x1A)
