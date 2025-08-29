// ============================================================================
// File: src/config/params.h
// ----------------------------------------------------------------------------
// Purpose: Displaygeometrie, Touch-Mapping & Gestenparameter
// ============================================================================
#pragma once
#include <Arduino.h>

// ---------------------------- Display Parameter ----------------------------
static constexpr uint16_t DISPLAY_WIDTH    = 320;
static constexpr uint16_t DISPLAY_HEIGHT   = 240;
static constexpr uint8_t  DISPLAY_ROTATION = 1; // 0..3

// ---------------------------- I2C Frequenzen --------------------------------
static constexpr uint32_t I2C_FREQ_HZ = 400000; // 400 kHz

// ---------------------------- Touch Mapping ---------------------------------
static constexpr int TOUCH_RAW_X_MIN = 0;
static constexpr int TOUCH_RAW_X_MAX = 240;
static constexpr int TOUCH_RAW_Y_MIN = 0;
static constexpr int TOUCH_RAW_Y_MAX = 320;

static constexpr bool TOUCH_SWAP_XY  = true;
static constexpr bool TOUCH_INVERT_X = false;
static constexpr bool TOUCH_INVERT_Y = true;

// ---------------------------- Gesten Parameter ------------------------------
static constexpr uint8_t  MAX_TOUCH_POINTS    = 5;
static constexpr uint16_t TAP_MAX_DURATION    = 250; // ms
static constexpr uint16_t TAP_MAX_MOVEMENT    = 20;  // px
static constexpr uint16_t DOUBLE_TAP_INTERVAL = 400; // ms
static constexpr uint16_t LONG_PRESS_DURATION = 800; // ms
static constexpr uint16_t SWIPE_MIN_DISTANCE  = 30;  // px
// Minimale Druck-/Signalstärke eines Touchpunkts (CST328 "strength")
static constexpr uint16_t TOUCH_MIN_STRENGTH = 20; // typ. 10..80; nach Bedarf anheben/absenken
// Wie lange muss sich die Fingeranzahl stabil halten, bevor die Geste ausgewertet wird?
static constexpr uint16_t TOUCH_SETTLE_MS    = 10;

// Erweiterte interne Schwellwerte
static constexpr float SWIPE_AXIS_RATIO  = 1.5f;   // Achsenbevorzugung
static constexpr float PINCH_THRESHOLD   = 18.0f;  // px Distanzänderung
static constexpr float ROTATE_THRESHOLD  = 15.0f;  // Grad
static constexpr float PROXIMITY_TOLER_PX= 60.0f;  // Matching Radius
