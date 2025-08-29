// ============================================================================
// File: src/touch/CST328Touch.h
// ----------------------------------------------------------------------------
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "../config/pins.h"
#include "../config/params.h"
#include "../core/types.h"

// CST328 Register (gemäß vorigem Sketch)
static constexpr uint16_t CST328_REG_NUM   = 0xD005;
static constexpr uint16_t CST328_REG_COORD = 0xD000;

struct RawCSTPoint { uint16_t x, y, strength; };

class CST328Touch {
public:
  bool begin();
  bool readFrame();
  void mapAndTrack();
  void getTouchPoints(TouchPoint out[MAX_TOUCH_POINTS]) const;
  uint8_t activeCount() const { return _activeCount; }

  // Interruptsteuerung
  static void IRAM_ATTR onIntISR();
  static volatile bool irqFlag;

private:
  bool readReg16(uint16_t reg, uint8_t* buf, size_t len);
  void rawToDisplay(uint16_t rx, uint16_t ry, uint16_t& dx, uint16_t& dy) const;
  void assignByProximity();

  RawCSTPoint _raw[MAX_TOUCH_POINTS]{};
  uint8_t _rawCount = 0;
  TouchPoint _points[MAX_TOUCH_POINTS]{};
  uint8_t _activeCount = 0;
};