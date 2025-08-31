
// ============================================================================
// File: src/display/DisplayManager.cpp
// ----------------------------------------------------------------------------
#include "DisplayManager.h"

bool DisplayManager::begin() {
  if (!_gfx.begin()) return false;
  _gfx.setRotation(DISPLAY_ROTATION);
  _gfx.fillScreen(TFT_BLACK);
  _gfx.setTextColor(TFT_WHITE, TFT_BLACK);
  _gfx.setTextSize(1);
  _gfx.setFont(&fonts::Font0);
  _gfx.setCursor(4, 4);
  _gfx.printf("Waveshare ESP32-S3 2.8\" – ST7789T3\n");
  _gfx.setCursor(4, 16);
  _gfx.printf("Display OK, Rotation=%d\n", DISPLAY_ROTATION);
  return true;
}

void DisplayManager::renderHUD(const GestureEvent& g, float fps,
                               float ax, float ay, float az,
                               float gx, float gy, float gz) {
  // Kopfbereich löschen
  _gfx.fillRect(0, 0, DISPLAY_WIDTH, 48, TFT_BLACK);
  _gfx.setCursor(4, 4);
  _gfx.printf("FPS: %.1f\n", fps);

  _gfx.setCursor(4, 16);
  _gfx.printf("IMU a[g]: %+.2f %+.2f %+.2f\n", ax, ay, az);

  _gfx.setCursor(4, 28);
  _gfx.printf("IMU g[dps]: %+.1f %+.1f %+.1f\n", gx, gy, gz);

  _gfx.fillRect(0, 48, DISPLAY_WIDTH, 12, TFT_DARKGREY);
  _gfx.setTextColor(TFT_YELLOW, TFT_DARKGREY);
  _gfx.setCursor(4, 50);
  const char* name = "None";
  switch (g.type) {
    case GestureType::Tap: name = "Tap"; break;
    case GestureType::DoubleTap: name = "DoubleTap"; break;
    case GestureType::LongPress: name = "LongPress"; break;
    case GestureType::SwipeLeft: name = "SwipeLeft"; break;
    case GestureType::SwipeRight: name = "SwipeRight"; break;
    case GestureType::SwipeUp: name = "SwipeUp"; break;
    case GestureType::SwipeDown: name = "SwipeDown"; break;
    case GestureType::PinchIn: name = "PinchIn"; break;
    case GestureType::PinchOut: name = "PinchOut"; break;
    case GestureType::RotateCW: name = "RotateCW"; break;
    case GestureType::RotateCCW: name = "RotateCCW"; break;
    case GestureType::TwoFingerTap: name = "TwoFingerTap"; break;
    case GestureType::ThreeFingerTap: name = "ThreeFingerTap"; break;
    default: name = "None"; break;
  }
  _gfx.printf("Gesture: %s (%u) val=%.2f [@%u,%u]",
              name, g.finger_count, g.value, g.x, g.y);
  _gfx.setTextColor(TFT_WHITE, TFT_BLACK);
}