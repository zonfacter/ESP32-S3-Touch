// ============================================================================
// File: src/display/DisplayManager.cpp - ERWEITERT FÜR TOUCH DEBUG
// ----------------------------------------------------------------------------
#include "DisplayManager.h"

bool DisplayManager::begin() {
  if (!_gfx.begin()) return false;

// Backlight einschalten
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, PIN_LCD_BL_ACTIVE_HIGH ? HIGH : LOW);

// Testbild (kurz) – hilft beim Inbetriebnehmen
  _gfx.fillScreen(TFT_BLUE); delay(150);
  _gfx.fillScreen(TFT_BLACK);
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

// NEU: Touch-Punkte visuell anzeigen
void DisplayManager::renderTouchPoints(const TouchPoint pts[MAX_TOUCH_POINTS], uint8_t activeCount) {
  // Touch area (unterhalb der HUD)
  const int TOUCH_AREA_TOP = 70;
  
  // Lösche vorherige Touch-Anzeige
  static uint16_t lastTouchX[MAX_TOUCH_POINTS] = {0};
  static uint16_t lastTouchY[MAX_TOUCH_POINTS] = {0};
  static bool lastActive[MAX_TOUCH_POINTS] = {false};
  
  // Lösche alte Positionen
  for (int i = 0; i < MAX_TOUCH_POINTS; i++) {
    if (lastActive[i] && lastTouchY[i] >= TOUCH_AREA_TOP) {
      _gfx.fillCircle(lastTouchX[i], lastTouchY[i], 8, TFT_BLACK);
    }
  }
  
  // Zeichne neue Touch-Punkte
  for (int i = 0; i < MAX_TOUCH_POINTS; i++) {
    if (pts[i].active) {
      uint16_t x = pts[i].x;
      uint16_t y = pts[i].y;
      
      // Nur im Touch-Bereich zeichnen
      if (y >= TOUCH_AREA_TOP && y < DISPLAY_HEIGHT && x < DISPLAY_WIDTH) {
        // Farbe je nach Punkt-Index
        uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_MAGENTA};
        uint16_t color = colors[i % 5];
        
        _gfx.fillCircle(x, y, 6, color);
        _gfx.drawCircle(x, y, 8, TFT_WHITE);
        
        // Touch-Info
        _gfx.setTextColor(TFT_WHITE, TFT_BLACK);
        _gfx.setCursor(x + 12, y - 4);
        _gfx.printf("%d", i);
      }
      
      lastTouchX[i] = x;
      lastTouchY[i] = y;
    }
    lastActive[i] = pts[i].active;
  }
  
  // Touch-Status unten anzeigen
  _gfx.fillRect(0, DISPLAY_HEIGHT - 20, DISPLAY_WIDTH, 20, TFT_NAVY);
  _gfx.setTextColor(TFT_WHITE, TFT_NAVY);
  _gfx.setCursor(4, DISPLAY_HEIGHT - 16);
  _gfx.printf("Touch Points: %d", activeCount);
  
  // Erste aktive Touch-Koordinaten anzeigen
  for (int i = 0; i < MAX_TOUCH_POINTS; i++) {
    if (pts[i].active) {
      _gfx.printf("  [%d] (%d,%d) S:%d", i, pts[i].x, pts[i].y, pts[i].strength);
      break; // Nur ersten anzeigen wegen Platz
    }
  }
  
  _gfx.setTextColor(TFT_WHITE, TFT_BLACK);
}