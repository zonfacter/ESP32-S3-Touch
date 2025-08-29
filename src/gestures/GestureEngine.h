// ============================================================================
// File: src/gestures/GestureEngine.h
// ----------------------------------------------------------------------------
#pragma once
#include <Arduino.h>
#include "../config/params.h"
#include "../core/types.h"

class GestureEngine {
public:
  void reset();
  GestureEvent process(const TouchPoint pts[MAX_TOUCH_POINTS], uint8_t activeCount);
private:
  // Double‑Tap Tracking
  unsigned long _lastTapTime = 0; uint16_t _lastTapX=0,_lastTapY=0;
  // Zwei‑Finger Aggregat
  bool _twoActive = false; bool _twoCommitted = false;
  float _twoStartDist = 0.f; float _twoStartAngle = 0.f;
};
