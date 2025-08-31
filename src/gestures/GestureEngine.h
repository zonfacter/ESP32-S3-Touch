// ============================================================================
// File: src/gestures/GestureEngine.h - VEREINFACHT & FUNKTIONIERT
// ----------------------------------------------------------------------------
#pragma once
#include <Arduino.h>
#include "../config/params.h"
#include "../core/types.h"

// Vereinfachte, aber funktionierende Gestenerkennung basierend auf
// ESP32_S3_CST328_Multi_Touch_Controller.ino
class GestureEngine {
public:
  void reset();
  
  // Gibt maximal EIN Event pro Aufruf zurück (None, wenn keins fällig ist)
  GestureEvent process(const TouchPoint pts[MAX_TOUCH_POINTS], uint8_t activeCount);

private:
  // ============================================
  // VEREINFACHTE STATE-VERWALTUNG
  // ============================================
  
  // Double-Tap Tracking
  unsigned long _lastTapTime = 0;
  uint16_t _lastTapX = 0;
  uint16_t _lastTapY = 0;
  
  // Long-Press Tracking
  unsigned long _lastLongPressTime = 0;
  
  // Frame-zu-Frame State
  uint8_t _lastActiveCount = 0;
  bool _lastActiveState[MAX_TOUCH_POINTS] = {false};
  
  // ============================================
  // PRIVATE HELPER-METHODEN
  // ============================================
  
  GestureEvent processSingleFingerGesture(const TouchPoint& tp, unsigned long now);
  GestureEvent processTwoFingerGesture(const TouchPoint& tp1, const TouchPoint& tp2, unsigned long now);
  GestureEvent processMultiFingerGesture(const TouchPoint pts[], uint8_t count, unsigned long now);
  GestureEvent checkLongPress(const TouchPoint& tp, unsigned long now);
};