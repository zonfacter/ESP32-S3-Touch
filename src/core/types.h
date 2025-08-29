// ============================================================================
// File: src/core/types.h
// ----------------------------------------------------------------------------
// Purpose: Gemeinsame Datentypen für Touchpunkte & Gestenereignisse
// ============================================================================
#pragma once
#include <Arduino.h>

enum class GestureType : uint8_t {
  None = 0,
  Tap,
  DoubleTap,
  LongPress,
  SwipeLeft,
  SwipeRight,
  SwipeUp,
  SwipeDown,
  PinchIn,
  PinchOut,
  RotateCW,
  RotateCCW,
  TwoFingerTap,
  ThreeFingerTap
};

struct TouchPoint {
  uint16_t x = 0, y = 0;
  uint16_t strength = 0;
  bool active = false;
  bool was_active_last_frame = false;
  unsigned long touch_start = 0;
  unsigned long touch_end = 0;
  uint16_t start_x = 0, start_y = 0;
  bool long_press_fired = false;
};

struct GestureEvent {
  GestureType type = GestureType::None;
  uint16_t x = 0, y = 0; // ggf. Mittelpunkt
  float value = 0.0f;    // z.B. Distanz-/Winkeländerung
  uint8_t finger_count = 0;
  unsigned long timestamp = 0;
};