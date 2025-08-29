// ============================================================================
// File: src/gestures/GestureEngine.cpp - VEREINFACHT & FUNKTIONIERT
// ----------------------------------------------------------------------------
#include "GestureEngine.h"
#include <math.h>

// ============================================
// BEWÄHRTE PARAMETER aus .ino
// ============================================
#define TAP_MAX_DURATION    250  // ms
#define TAP_MAX_MOVEMENT    20   // px
#define DOUBLE_TAP_INTERVAL 400  // ms  
#define LONG_PRESS_DURATION 800  // ms
#define SWIPE_MIN_DISTANCE  30   // px

void GestureEngine::reset(){
  _lastTapTime = 0;
  _lastTapX = 0;
  _lastTapY = 0;
  _lastLongPressTime = 0;
  
  // Touch-Points zurücksetzen
  for(int i = 0; i < MAX_TOUCH_POINTS; i++){
    _lastActiveState[i] = false;
  }
  _lastActiveCount = 0;
}

GestureEvent GestureEngine::process(const TouchPoint pts[MAX_TOUCH_POINTS], uint8_t activeCount){
  GestureEvent g;
  g.type = GestureType::None;
  g.timestamp = millis();
  g.finger_count = activeCount;
  
  unsigned long now = millis();
  
  // ============================================
  // VEREINFACHTE LOGIK: TOUCH BEENDET → GESTEN AUSWERTEN
  // ============================================
  
  if(_lastActiveCount > 0 && activeCount == 0){
    // Touch wurde beendet - jetzt Gesten auswerten
    
    if(_lastActiveCount == 1){
      // Ein-Finger-Gesten
      g = processSingleFingerGesture(pts[0], now);
      
    } else if(_lastActiveCount == 2){
      // Zwei-Finger-Gesten  
      g = processTwoFingerGesture(pts[0], pts[1], now);
      
    } else if(_lastActiveCount >= 3){
      // Multi-Finger-Gesten
      g = processMultiFingerGesture(pts, _lastActiveCount, now);
    }
  }
  
  // ============================================
  // LONG-PRESS: Live während Touch (nur einmalig)
  // ============================================
// Einmaliger Long Press pro Touch-Session
static unsigned long lastLongPressTouch = 0;
if(duration > LONG_PRESS_DURATION && tp.touch_start != lastLongPressTouch) {
  g.type = GestureType::LongPress;
  lastLongPressTouch = tp.touch_start; // Merken für diese Session
}
  
  // State für nächsten Frame speichern
  _lastActiveCount = activeCount;
  for(int i = 0; i < MAX_TOUCH_POINTS; i++){
    _lastActiveState[i] = pts[i].active;
  }
  
  return g;
}

// ============================================
// EIN-FINGER-GESTEN (vereinfacht aus .ino)
// ============================================
GestureEvent GestureEngine::processSingleFingerGesture(const TouchPoint& tp, unsigned long now){
  GestureEvent g;
  g.type = GestureType::None;
  g.timestamp = now;
  g.finger_count = 1;
  g.x = tp.x;
  g.y = tp.y;
  
  if(!tp.was_active_last_frame) return g; // Nicht von letztem Frame
  
  unsigned long duration = tp.touch_end - tp.touch_start;
  float movement = sqrt(pow(tp.x - tp.start_x, 2) + pow(tp.y - tp.start_y, 2));
  
  if(movement <= TAP_MAX_MOVEMENT && duration <= TAP_MAX_DURATION){
    // TAP oder DOUBLE_TAP
    if(now - _lastTapTime < DOUBLE_TAP_INTERVAL &&
       abs(tp.x - _lastTapX) < TAP_MAX_MOVEMENT &&
       abs(tp.y - _lastTapY) < TAP_MAX_MOVEMENT){
      // DOUBLE TAP erkannt
      g.type = GestureType::DoubleTap;
      g.value = 2;
      _lastTapTime = 0; // Reset um Triple-Tap zu vermeiden
    } else {
      // SINGLE TAP
      g.type = GestureType::Tap;  
      g.value = 1;
      _lastTapTime = now;
      _lastTapX = tp.x;
      _lastTapY = tp.y;
    }
    
  } else if(movement > SWIPE_MIN_DISTANCE && duration < 500){
    // SWIPE
    float dx = tp.x - tp.start_x;
    float dy = tp.y - tp.start_y;
    
    if(abs(dx) > abs(dy)){
      if(dx > 0){
        g.type = GestureType::SwipeRight;
      } else {
        g.type = GestureType::SwipeLeft;
      }
    } else {
      if(dy > 0){
        g.type = GestureType::SwipeDown;
      } else {
        g.type = GestureType::SwipeUp;
      }
    }
    g.value = movement;
  }
  
  return g;
}

// ============================================
// ZWEI-FINGER-GESTEN (vereinfacht)
// ============================================
GestureEvent GestureEngine::processTwoFingerGesture(const TouchPoint& tp1, const TouchPoint& tp2, unsigned long now){
  GestureEvent g;
  g.type = GestureType::TwoFingerTap;
  g.timestamp = now;
  g.finger_count = 2;
  g.x = (tp1.x + tp2.x) / 2;
  g.y = (tp1.y + tp2.y) / 2;
  g.value = 2;
  
  // TODO: Hier könnten Pinch/Rotate-Gesten hinzugefügt werden
  // Erstmal einfach Two-Finger-Tap für Stabilität
  
  return g;
}

// ============================================
// MULTI-FINGER-GESTEN
// ============================================  
GestureEvent GestureEngine::processMultiFingerGesture(const TouchPoint pts[], uint8_t count, unsigned long now){
  GestureEvent g;
  g.type = GestureType::ThreeFingerTap;
  g.timestamp = now;
  g.finger_count = count;
  g.value = count;
  
  // Zentrum berechnen
  uint16_t center_x = 0, center_y = 0;
  for(int i = 0; i < count; i++){
    center_x += pts[i].x;
    center_y += pts[i].y;  
  }
  g.x = center_x / count;
  g.y = center_y / count;
  
  return g;
}

// ============================================
// LONG-PRESS: Live während Touch
// ============================================
GestureEvent GestureEngine::checkLongPress(const TouchPoint& tp, unsigned long now){
  GestureEvent g;
  g.type = GestureType::None;
  g.timestamp = now;
  
  if(!tp.active) return g;
  
  unsigned long duration = now - tp.touch_start;
  float movement = sqrt(pow(tp.x - tp.start_x, 2) + pow(tp.y - tp.start_y, 2));
  
  if(duration > LONG_PRESS_DURATION && movement <= TAP_MAX_MOVEMENT){
    // Long Press erkannt - aber nur einmalig pro Touch-Session
    if(now - _lastLongPressTime > 1000){
      g.type = GestureType::LongPress;
      g.x = tp.x;
      g.y = tp.y;
      g.value = duration;
      g.finger_count = 1;
      _lastLongPressTime = now;
    }
  }
  
  return g;
}