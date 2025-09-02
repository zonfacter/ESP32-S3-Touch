#include "QMI8658.h"

// Register (Auszug)
static constexpr uint8_t REG_WHO_AM_I = 0x00;
static constexpr uint8_t REG_CTRL1    = 0x02; // ADDR_AI etc.
static constexpr uint8_t REG_CTRL2    = 0x03; // Acc FS/ODR
static constexpr uint8_t REG_CTRL3    = 0x04; // Gyro FS/ODR
static constexpr uint8_t REG_CTRL5    = 0x06; // LPF
static constexpr uint8_t REG_CTRL7    = 0x08; // aEN/gEN
static constexpr uint8_t REG_STATUS0  = 0x2E; // aDA/gDA
static constexpr uint8_t REG_AX_L     = 0x35; // ... bis 0x40

static constexpr uint8_t WHOAMI_EXPECT = 0x05;       // :contentReference[oaicite:9]{index=9}

bool QMI8658::begin() {
  // I2C erst in App gesetzt; hier nur Adresse finden & konfigurieren
  if (!detectAddress()) {
    Serial.println("[IMU] WHO_AM_I mismatch / kein Sensor");
    return false;
  }

  // --- Soft Reset (Reset-Register 0x60) ---  :contentReference[oaicite:10]{index=10}
  write1(0x60, 0xB0);   // gängiger Wert für Soft-Reset
  delay(10);

  // --- CTRL1: Auto-Increment aktivieren (ADDR_AI=1), Little Endian (BE=0) ---  :contentReference[oaicite:11]{index=11}
  // Bit6=1 (ADDR_AI), Bit5=0 (BE little-endian), Rest 0 => 0b0100'0000 = 0x40
  if (!write1(REG_CTRL1, 0x40)) return false;

  // --- CTRL2: Acc FS/ODR (±4g @ 500 Hz) ---  :contentReference[oaicite:12]{index=12}
  // aFS=001 (±4g) -> Bits6..4=0b001; aODR=0100 (500 Hz) -> Bits3..0=0b0100 => 0x14
  if (!write1(REG_CTRL2, 0x14)) return false;

  // --- CTRL3: Gyro FS/ODR (±2000 dps @ 470 Hz) ---  :contentReference[oaicite:13]{index=13}
  // gFS=111 (±2048 dps ≈ 2000 dps) -> Bits6..4=0b111; gODR=0100 (470 Hz) -> Bits3..0=0b0100 => 0x74
  if (!write1(REG_CTRL3, 0x74)) return false;

  // --- CTRL5: LPF optional (hier aus, 0x00). Später feintunen. ---  :contentReference[oaicite:14]{index=14}
  write1(REG_CTRL5, 0x00);

  // --- CTRL7: aEN/gEN aktiv ---  :contentReference[oaicite:15]{index=15}
  // syncSmpl=0 (einfach), gEN=1, aEN=1 => 0b0000'0011 = 0x03
  if (!write1(REG_CTRL7, 0x03)) return false;

  delay(5);
  Serial.printf("[IMU] QMI8658 @0x%02X initialisiert\n", _addr);
  return true;
}

bool QMI8658::read(IMUData& out) {
  // Daten bereit? STATUS0: Bit0=aDA, Bit1=gDA  :contentReference[oaicite:16]{index=16}
  uint8_t st = 0;
  if (!readN(REG_STATUS0, &st, 1)) return false;
  if ((st & 0x03) == 0) return false; // nichts Neues

  // Dank ADDR_AI=1 können wir 12 Bytes am Stück holen (AX..GZ)  :contentReference[oaicite:17]{index=17}
  uint8_t b[12];
  if (!readN(REG_AX_L, b, sizeof(b))) return false;

  auto s16 = [&](int i) -> int16_t { return (int16_t)((uint16_t)b[i+1] << 8 | b[i]); };

  int16_t ax = s16(0), ay = s16(2), az = s16(4);
  int16_t gx = s16(6), gy = s16(8), gz = s16(10);

  // Skalen: ±4g => 8192 LSB/g; ±2000dps => 16.4 LSB/(°/s)  :contentReference[oaicite:18]{index=18} :contentReference[oaicite:19]{index=19}
  constexpr float ACC_LSB_PER_G = 8192.0f;
  constexpr float GYR_LSB_PER_DPS = 16.4f;

  out.ax = ax / ACC_LSB_PER_G;
  out.ay = ay / ACC_LSB_PER_G;
  out.az = az / ACC_LSB_PER_G;

  out.gx = gx / GYR_LSB_PER_DPS;
  out.gy = gy / GYR_LSB_PER_DPS;
  out.gz = gz / GYR_LSB_PER_DPS;

  return true;
}

bool QMI8658::detectAddress() {
  for (uint8_t cand : { 0x6B, 0x6A }) {
    _addr = cand;
    uint8_t id = 0;
    if (readN(REG_WHO_AM_I, &id, 1) && id == WHOAMI_EXPECT) {
      Serial.printf("[IMU] WHO_AM_I=0x%02X @0x%02X OK\n", id, cand);
      return true;
    }
  }
  _addr = 0;
  return false;
}

// --- I2C helpers ---
bool QMI8658::write1(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool QMI8658::readN(uint8_t reg, uint8_t* buf, size_t n) {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  size_t got = Wire.requestFrom((int)_addr, (int)n, (int)true);
  if (got != n) return false;
  for (size_t i=0; i<n; ++i) {
    if (!Wire.available()) return false;
    buf[i] = Wire.read();
  }
  return true;
}
