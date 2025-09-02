// ============================================================================
// File: src/config/params.h - KORRIGIERTE TOUCH PARAMETER
// ----------------------------------------------------------------------------
#pragma once
#include <Arduino.h>

// ---------------------------- Display Parameter ----------------------------
static constexpr uint16_t DISPLAY_WIDTH    = 320;
static constexpr uint16_t DISPLAY_HEIGHT   = 240;
static constexpr uint8_t  DISPLAY_ROTATION = 1; // 0..3

// ---------------------------- I2C Frequenzen --------------------------------
static constexpr uint32_t I2C_FREQ_HZ = 400000; // 400 kHz

// ---------------------------- Touch Mapping - KORRIGIERT -------------------
static constexpr int  TOUCH_RAW_X_MIN = 0;
static constexpr int  TOUCH_RAW_X_MAX = 4095;  // volle 12-bit Range
static constexpr int  TOUCH_RAW_Y_MIN = 0;  
static constexpr int  TOUCH_RAW_Y_MAX = 4095;

static constexpr bool TOUCH_SWAP_XY   = true;   // wie in deiner funktionierenden Config
static constexpr bool TOUCH_INVERT_X  = false;
static constexpr bool TOUCH_INVERT_Y  = true;


// ---------------------------- Gesten Parameter - OPTIMIERT -----------------
static constexpr uint8_t  MAX_TOUCH_POINTS    = 5;
static constexpr uint16_t TAP_MAX_DURATION    = 300; // ms
static constexpr uint16_t TAP_MAX_MOVEMENT    = 40;  // px - toleranter
static constexpr uint16_t DOUBLE_TAP_INTERVAL = 500; // ms
static constexpr uint16_t LONG_PRESS_DURATION = 700; // ms - nicht zu schnell
static constexpr uint16_t SWIPE_MIN_DISTANCE  = 30;  // px
// Touch-Qualität
static constexpr uint16_t TOUCH_MIN_STRENGTH = 0;  // nach Bedarf anpassen
static constexpr uint16_t TOUCH_SETTLE_MS    = 20;   // ms - mehr Stabilität

// Erweiterte interne Schwellwerte
static constexpr float SWIPE_AXIS_RATIO  = 1.3f;   
static constexpr float PINCH_THRESHOLD   = 15.0f;  // px
static constexpr float ROTATE_THRESHOLD  = 12.0f;  // Grad
static constexpr float PROXIMITY_TOLER_PX= 60.0f;  // px