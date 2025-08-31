// ============================================================================
// File: src/imu/QMI8658.h
// ----------------------------------------------------------------------------
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "../config/pins.h"
#include "../config/params.h"   // <-- hinzufügen, damit I2C_FREQ_HZ sichtbar ist

struct IMUData { float ax, ay, az, gx, gy, gz; };

class QMI8658 {
public:
  bool begin();
  bool read(IMUData& out);
private:
  bool write1(uint8_t reg, uint8_t val);
  bool readN(uint8_t reg, uint8_t* buf, size_t n);
  uint8_t _addr = 0x6B; // üblich für QMI8658C
};