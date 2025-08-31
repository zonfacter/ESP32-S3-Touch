// ============================================================================
// File: src/audio/AudioI2S.h
// ----------------------------------------------------------------------------
#pragma once
#include <Arduino.h>
#include "../config/pins.h"
#include "../core/types.h"

class AudioI2S {
public:

  bool begin(uint32_t sampleRate = 22050);
  void playGesture(const GestureType g);
private:
  void toneHz(float hz, uint16_t ms, float amp=0.2f);
  unsigned long _gateUntil = 0;   // einfache Rate-Limitierung
};