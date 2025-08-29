// ============================================================================
// File: src/gestures/GestureEngine.h
// ----------------------------------------------------------------------------
#pragma once
#include <Arduino.h>
#include "../config/params.h"
#include "../core/types.h"


// State‑Machine‑basierte Gestenerkennung: Events feuern genau einmal
class GestureEngine {
public:
void reset();
// Gibt maximal EIN Event pro Aufruf zurück (None, wenn keins fällig ist)
GestureEvent process(const TouchPoint pts[MAX_TOUCH_POINTS], uint8_t activeCount);


private:
// 1‑Finger Zustand
bool one_active = false; // ob gerade 1 Finger verfolgt wird
bool one_committed = false; // LongPress/Swipe bereits gemeldet
unsigned long one_t0 = 0; // Startzeit des 1‑Finger Kontakts
uint16_t one_sx = 0, one_sy = 0; // Startposition
uint16_t one_lx = 0, one_ly = 0; // letzte Position


// 2‑Finger Zustand
bool two_active = false; // ob 2 Finger aktiv sind
bool two_committed = false; // Pinch/Rotate bereits gemeldet
float two_d0 = 0.f; // Startdistanz
float two_a0 = 0.f; // Startwinkel (rad)
unsigned long two_t0 = 0; // Startzeit 2‑Finger Phase
uint16_t two_cx = 0, two_cy = 0; // Mittelpunkt


// Double‑Tap Tracking (letzter Tap‑Zeitpunkt/Ort)
unsigned long lastTapTime = 0;
uint16_t lastTapX = 0;
uint16_t lastTapY = 0;
};