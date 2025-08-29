// ============================================================================
// File: src/imu/QMI8658.cpp
// ----------------------------------------------------------------------------
#include "QMI8658.h"

bool QMI8658::begin(){
  Wire.begin(PIN_IMU_SDA, PIN_IMU_SCL, I2C_FREQ_HZ);
  // Soft init: Reset + basic config (vereinfachte Demo – robust genug zum Lesen)
  write1(0x60, 0x01); // soft reset
  delay(10);
  // Set ODR & Range (vereinfachte Standardwerte)
  write1(0x02, 0x60); // CTRL1: Acc ODR ≈ 200Hz, ±4g
  write1(0x03, 0x60); // CTRL2: Gyro ODR ≈ 200Hz, ±2000dps
  write1(0x06, 0x03); // CTRL5: Acc/Gyro enable
  delay(10);
  return true;
}

bool QMI8658::write1(uint8_t reg, uint8_t val){
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission()==0;
}

bool QMI8658::readN(uint8_t reg, uint8_t* buf, size_t n){
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  if (Wire.endTransmission(false)!=0) return false;
  size_t got = Wire.requestFrom((int)_addr, (int)n, (int)true);
  if (got!=n) return false;
  for (size_t i=0;i<n;i++){ if(!Wire.available()) return false; buf[i]=Wire.read(); }
  return true;
}

bool QMI8658::read(IMUData& out){
  // Burst ab OUTX_L_A (0x35) -> 12 Bytes: Acc(L/H)*3 + Gyro(L/H)*3
  uint8_t b[12];
  if (!readN(0x35, b, 12)) return false;
  auto s16=[&](int i){ return (int16_t)((b[i+1]<<8)|b[i]); };
  int16_t ax=s16(0), ay=s16(2), az=s16(4);
  int16_t gx=s16(6), gy=s16(8), gz=s16(10);

  // Rohskalierung (vereinfachte Standard-Skalen; ggf. anpassen)
  const float accLSB = 1.0f/8192.0f; // ±4g
  const float gyrLSB = 1.0f/16.4f;   // ±2000 dps (≈16.4 LSB/°/s)
  out.ax = ax * accLSB; out.ay = ay * accLSB; out.az = az * accLSB;
  out.gx = gx / 16.4f; out.gy = gy / 16.4f; out.gz = gz / 16.4f;
  return true;
}
