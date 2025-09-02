#pragma once
#include <Arduino.h>
#include <Wire.h>

// Minimaler Datenträger für App
struct IMUData {
  float ax, ay, az;   // g
  float gx, gy, gz;   // dps
};

class QMI8658 {
public:
  bool begin();                  // init + config
  bool read(IMUData& out);       // eine Probe lesen (true = Daten geliefert)

private:
  uint8_t _addr = 0x00;

  bool write1(uint8_t reg, uint8_t val);
  bool readN(uint8_t reg, uint8_t* buf, size_t n);
  bool detectAddress();          // 0x6B -> 0x6A via WHO_AM_I
};
