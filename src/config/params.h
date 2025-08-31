// ============================================================================
// File: src/config/params.h - BEWÄHRTE WERTE aus .ino
// ----------------------------------------------------------------------------
// Purpose: Displaygeometrie, Touch-Mapping & Gestenparameter
// VEREINFACHT & FUNKTIONIERT - basierend auf ESP32_S3_CST328_Multi_Touch_Controller.ino
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

// ---------------------------- Gesten Parameter (BEWÄHRT!) ------------------
static constexpr uint8_t  MAX_TOUCH_POINTS    = 5;

// ✅ BEWÄHRTE SCHWELLENWERTE aus .ino (funktionieren garantiert!)
static constexpr uint16_t TAP_MAX_DURATION    = 250; // ms
static constexpr uint16_t TAP_MAX_MOVEMENT    = 20;  // px  
static constexpr uint16_t DOUBLE_TAP_INTERVAL = 400; // ms
static constexpr uint16_t LONG_PRESS_DURATION = 800; // ms
static constexpr uint16_t SWIPE_MIN_DISTANCE  = 30;  // px

// ❌ ENTFERNTE PROBLEMATISCHE PARAMETER:
// - TOUCH_MIN_STRENGTH (CST328Touch soll das intern handhaben)
// - TOUCH_SETTLE_MS (macht Gesten langsam und unzuverlässig)
// - SWIPE_AXIS_RATIO (zu komplex, einfache abs() Vergleiche reichen)
// - PINCH_THRESHOLD (erstmal weglassen bis Pinch implementiert)
// - ROTATE_THRESHOLD (erstmal weglassen bis Rotate implementiert)
// - PROXIMITY_TOLER_PX (Proximity-Tracking ist zu komplex)

// ---------------------------- Noch benötigte Parameter (für CST328Touch) -------
// Diese Parameter werden noch vom bestehenden CST328Touch Code verwendet
static constexpr uint16_t TOUCH_MIN_STRENGTH = 20;   // CST328 strength filter
static constexpr float PROXIMITY_TOLER_PX = 60.0f;   // Touch-Point matching
// Anti-Phantom Touch Filter
static constexpr uint8_t TOUCH_EDGE_MARGIN = 5; // px vom Rand
static constexpr uint16_t PHANTOM_FILTER_STRENGTH = 40; // Mindest-Stärke für Edge-Touches
static constexpr uint16_t TOUCH_SETTLE_MS = 5; 
// ---------------------------- Performance Parameter -------------------------
static constexpr uint8_t  TARGET_FPS = 60;
static constexpr uint16_t FRAME_TIME_MS = 1000 / TARGET_FPS; // ~17ms