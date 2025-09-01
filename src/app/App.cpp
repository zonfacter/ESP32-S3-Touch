// ============================================================================
// File: src/app/App.cpp
// ----------------------------------------------------------------------------
#include "App.h"
#include "../config/pins.h"
#include "../config/params.h"

bool App::begin(){
  Serial.begin(115200);
 
  // Warte kurz auf USB-CDC (ohne „hängenzubleiben“, max. 2s)
  unsigned long t0 = millis();
  while (!Serial && (millis() - t0) < 2000) { delay(10); }
  delay(50);
  Serial.println("\n[APP] Start");

  // I2C Busses
  Wire.begin(PIN_IMU_SDA, PIN_IMU_SCL, I2C_FREQ_HZ);
  Wire1.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL, I2C_FREQ_HZ);
  delay(50);
  scanI2C(Wire,  "I2C0(IMU)");
  scanI2C(Wire1, "I2C1(TOUCH)");

  if (!_disp.begin()) { Serial.println("Display init failed"); return false; }
  if (!_touch.begin()) Serial.println("Warn: Touch init returned false (weiter mit Fallback)");
  if (!_imu.begin())   Serial.println("Warn: IMU init returned false (weiter)");
  if (!_audio.begin()) Serial.println("Warn: Audio init returned false (weiter)");

  _rs485.begin(115200); // Standardbaud, anpassbar
  _console.begin(115200);
  _console.onLine([this](const String& line){
    if (line.startsWith("rs485send ")){
      String payload = line.substring(10);
      _rs485.write(payload + "\r\n");                 // ← Zeilenende mitsenden
      Serial.println("[RS485] TX: " + payload);
    }
    else if (line.startsWith("rs485baud ")){
      uint32_t b = line.substring(10).toInt();
      if (b > 0){
        _rs485.setBaud(b);
        Serial.printf("[RS485] Baud -> %u\n", b);     // ← \n ergänzt
      }
    }
    else if (line == "rs485echo on"){
      _echo485 = true;  Serial.println("[RS485] Echo ON");
    }
    else if (line == "rs485echo off"){
      _echo485 = false; Serial.println("[RS485] Echo OFF");
    }
    else {
      Serial.println("Commands: rs485send <text> | rs485baud <n> | rs485echo on|off");
    }
  }); // ← fehlendes ');'

  _gest.reset();
  _lastGesture.type = GestureType::None;
  _lastFrame = millis();
  return true;
}

void App::scanI2C(TwoWire& w, const char* name){
  Serial.printf("[SCAN] %s...\n", name);
  for (uint8_t a=1; a<127; a++){
    w.beginTransmission(a);
    if (w.endTransmission()==0){
      Serial.printf("  found 0x%02X\n", a);
    }
  }
  Serial.flush();
}

// ============================================================================
// App::loop() - EXAKTES .ino-Timing und Calling-Pattern
// ============================================================================

void App::loop(){
  static unsigned long lastUpdate = 0;
  static unsigned long frameCount = 0;
  static unsigned long lastFpsReport = 0;
  
  unsigned long now = millis();
  
  // EXAKT wie .ino: NUR alle 17ms updaten (60fps)
  if (now - lastUpdate >= 17) {
    lastUpdate = now;
    frameCount++;
    
    // FPS berechnen
    float dt = (now - _lastFrame) / 1000.0f; 
    if (dt < 1e-6f) dt = 1e-6f;
    _fps = 0.9f * _fps + 0.1f * (1.0f / dt); 
    _lastFrame = now;
    
    // EINFACHE Touch-Verarbeitung - KEIN Debug-Spam
    updateMultiTouch();
    
    // HUD-Update
    _disp.renderHUD(_lastGesture, _fps,
                    _imuData.ax, _imuData.ay, _imuData.az,
                    _imuData.gx, _imuData.gy, _imuData.gz);
    
    // FPS-Report alle 5 Sekunden (wie .ino)
    if (now - lastFpsReport >= 5000) {
      float fps = frameCount / 5.0;
      Serial.printf("FPS: %.1f | Aktive Touches: %d | Heap: %uKB\n", 
                   fps, _touch.activeCount(), ESP.getFreeHeap() / 1024);
      frameCount = 0;
      lastFpsReport = now;
    }
  }
  
  // IMU lesen (entspannt 50 Hz)
  static unsigned long lastIMU = 0;
  if (now - lastIMU >= 20){ 

    _imu.read(_imuData); 
    lastIMU = now; 
  }
  
  // Konsole & RS485
  _console.loop();
  
  // RS485 RX
  static char rxbuf[256]; 
  static size_t rxi = 0;
  while (_rs485.available() > 0){
    int c = _rs485.read(); 
    if (c < 0) break;
    if (c == '\r') continue;
    if (rxi < sizeof(rxbuf) - 1) rxbuf[rxi++] = (char)c;
    if (c == '\n'){
      rxbuf[rxi] = 0;
      String line = String(rxbuf);
      if (_echo485) _rs485.write(line);
      Serial.print("[RS485] RX: "); Serial.print(line);
      rxi = 0;
    }
  }
  
  // Minimale Delay wie .ino
  delay(1);
}

// ============================================================================
// Komplettes Gesten-System - ALLE Gesten implementiert
// ============================================================================

void App::updateMultiTouch(){
  unsigned long now = millis();
  static unsigned long lastPoll = 0;
  static uint8_t last_active_count = 0;
  
  // Single-Finger Gesture States
  static bool swipeCommitted = false;
  static bool suppressTapOnRelease = false;
  
  // Two-Finger Gesture States
  static bool twoFingerGestureActive = false;
  static bool twoFingerCommitted = false;
  static float initialDistance = 0.0f;
  static float initialAngle = 0.0f;
  
  if (now - lastPoll >= 17) {
    lastPoll = now;
    
    TouchPoint pts[MAX_TOUCH_POINTS]; 
    _touch.getTouchPoints(pts);
    
    for (int i = 0; i < MAX_TOUCH_POINTS; i++) {
      pts[i].was_active_last_frame = pts[i].active;
    }
    
    bool hasTouch = _touch.readFrame();
    if (hasTouch) {
      _touch.mapAndTrack();
    }
    
    _touch.getTouchPoints(pts);
    uint8_t current_active_count = _touch.activeCount();
    
    for(int i = 0; i < MAX_TOUCH_POINTS; i++){
      if(pts[i].was_active_last_frame && !pts[i].active){
        pts[i].touch_end = now;
      }
    }
    
    // ============================================
    // EIN-FINGER-GESTEN (Live während Touch)
    // ============================================
    if (current_active_count == 1) {
      int idx = -1; 
      for (int i = 0; i < MAX_TOUCH_POINTS; i++) {
        if (pts[i].active) { idx = i; break; }
      }
      
      if (idx >= 0) {
        TouchPoint &tp = pts[idx];
        unsigned long duration = now - tp.touch_start;
        float dx = (float)tp.x - (float)tp.start_x;
        float dy = (float)tp.y - (float)tp.start_y;
        float movement = sqrt(dx*dx + dy*dy);
        
        // LIVE SWIPE-ERKENNUNG
        if (!swipeCommitted && movement >= 30 && duration <= 800) {
          GestureType t = GestureType::None;
          
          if (abs(dx) > abs(dy) * 1.5f) {
            t = (dx > 0) ? GestureType::SwipeRight : GestureType::SwipeLeft;
          } else if (abs(dy) > abs(dx) * 1.5f) {
            t = (dy > 0) ? GestureType::SwipeDown : GestureType::SwipeUp;
          }
          
          if (t != GestureType::None) {
            Serial.printf("LIVE SWIPE: ");
            switch(t) {
              case GestureType::SwipeLeft: Serial.print("LEFT"); break;
              case GestureType::SwipeRight: Serial.print("RIGHT"); break;
              case GestureType::SwipeUp: Serial.print("UP"); break;
              case GestureType::SwipeDown: Serial.print("DOWN"); break;
            }
            Serial.printf(" at (%d,%d) dist=%.1f\n", tp.x, tp.y, movement);
            
            setGesture(t, tp.x, tp.y, movement, 1, now);
            swipeCommitted = true;
            suppressTapOnRelease = true;
          }
        }
        
        // LONG PRESS
        if (duration > 800 && movement <= 20) {
          static unsigned long last_long_press = 0;
          if (now - last_long_press > 1000) {
            Serial.printf("GESTE: LONG_PRESS at (%d,%d) duration=%lu\n", tp.x, tp.y, duration);
            setGesture(GestureType::LongPress, tp.x, tp.y, duration, 1, now);
            last_long_press = now;
          }
        }
      }
    }
    
    // ============================================
    // ZWEI-FINGER-GESTEN (Pinch & Rotate)
    // ============================================
    else if (current_active_count == 2) {
      int finger1 = -1, finger2 = -1;
      for (int i = 0; i < MAX_TOUCH_POINTS; i++) {
        if (pts[i].active) {
          if (finger1 == -1) finger1 = i;
          else if (finger2 == -1) { finger2 = i; break; }
        }
      }
      
      if (finger1 >= 0 && finger2 >= 0) {
        TouchPoint &tp1 = pts[finger1];
        TouchPoint &tp2 = pts[finger2];
        
        float currentDistance = sqrt(pow(tp2.x - tp1.x, 2) + pow(tp2.y - tp1.y, 2));
        float currentAngle = atan2(tp2.y - tp1.y, tp2.x - tp1.x);
        uint16_t centerX = (tp1.x + tp2.x) / 2;
        uint16_t centerY = (tp1.y + tp2.y) / 2;
        
        if (!twoFingerGestureActive) {
          // Zwei-Finger-Geste beginnt
          twoFingerGestureActive = true;
          twoFingerCommitted = false;
          initialDistance = currentDistance;
          initialAngle = currentAngle;
          suppressTapOnRelease = true; // Verhindert später Tap-Erkennung
        }
        
        if (!twoFingerCommitted) {
          float distanceChange = currentDistance - initialDistance;
          float angleChange = currentAngle - initialAngle;
          
          // Winkel-Normalisierung
          while (angleChange > M_PI) angleChange -= 2 * M_PI;
          while (angleChange < -M_PI) angleChange += 2 * M_PI;
          
          // PINCH-ERKENNUNG
          if (abs(distanceChange) >= 25.0f) { // Mindest-Distanz-Änderung
            if (distanceChange > 0) {
              Serial.printf("GESTE: PINCH_OUT (Zoom In) at (%d,%d) dist_change=%.1f\n", 
                           centerX, centerY, distanceChange);
              setGesture(GestureType::PinchOut, centerX, centerY, distanceChange, 2, now);
            } else {
              Serial.printf("GESTE: PINCH_IN (Zoom Out) at (%d,%d) dist_change=%.1f\n", 
                           centerX, centerY, abs(distanceChange));
              setGesture(GestureType::PinchIn, centerX, centerY, abs(distanceChange), 2, now);
            }
            twoFingerCommitted = true;
          }
          
          // ROTATE-ERKENNUNG
          else if (abs(angleChange) >= (15.0f * M_PI / 180.0f)) { // 15 Grad in Radiant
            float angleDegrees = angleChange * 180.0f / M_PI;
            if (angleChange > 0) {
              Serial.printf("GESTE: ROTATE_CCW at (%d,%d) angle=%.1f deg\n", 
                           centerX, centerY, angleDegrees);
              setGesture(GestureType::RotateCCW, centerX, centerY, abs(angleDegrees), 2, now);
            } else {
              Serial.printf("GESTE: ROTATE_CW at (%d,%d) angle=%.1f deg\n", 
                           centerX, centerY, abs(angleDegrees));
              setGesture(GestureType::RotateCW, centerX, centerY, abs(angleDegrees), 2, now);
            }
            twoFingerCommitted = true;
          }
        }
      }
    }
    
    // ============================================
    // TOUCH BEENDET - Release-Gesten & Reset
    // ============================================
    else if (current_active_count == 0) {
      // Reset aller Gesture-States
      swipeCommitted = false;
      twoFingerGestureActive = false;
      twoFingerCommitted = false;
      
      // Release-Gesten nur wenn nichts live erkannt wurde
      if (last_active_count > 0 && !suppressTapOnRelease) {
        processReleaseGestures(pts, last_active_count, now);
      }
      suppressTapOnRelease = false;
    }
    
    // ============================================
    // DREI+ FINGER - Sofortige Multi-Touch-Erkennung
    // ============================================
    else if (current_active_count >= 3 && last_active_count < 3) {
      // Multi-Touch-Geste sofort bei Erkennung
      uint16_t center_x = 0, center_y = 0;
      for(int i = 0; i < MAX_TOUCH_POINTS; i++){
        if(pts[i].active) {
          center_x += pts[i].x;
          center_y += pts[i].y;
        }
      }
      center_x /= current_active_count;
      center_y /= current_active_count;
      
      Serial.printf("GESTE: MULTI_FINGER_TAP fingers=%d at (%d,%d)\n", 
                   current_active_count, center_x, center_y);
      setGesture(GestureType::ThreeFingerTap, center_x, center_y, current_active_count, current_active_count, now);
      suppressTapOnRelease = true;
    }
    
    // Display-Reset nach 3 Sekunden
    if (_lastGesture.type != GestureType::None && 
        now - _lastGesture.timestamp > 3000) {
      _lastGesture.type = GestureType::None;
    }
    
    last_active_count = current_active_count;
  }
}

// ============================================================================
// HELPER: Gesture setzen (DRY-Prinzip)
// ============================================================================
void App::setGesture(GestureType type, uint16_t x, uint16_t y, float value, uint8_t fingers, unsigned long timestamp) {
  _lastGesture.type = type;
  _lastGesture.x = x;
  _lastGesture.y = y;
  _lastGesture.value = value;
  _lastGesture.finger_count = fingers;
  _lastGesture.timestamp = timestamp;
}

// ============================================================================
// Release-Gesten (nur Tap/DoubleTap)
// ============================================================================
void App::processReleaseGestures(TouchPoint pts[], uint8_t last_count, unsigned long now) {
  static unsigned long last_tap_time = 0;
  static uint16_t last_tap_x = 0, last_tap_y = 0;
  
  if (last_count == 1) {
    TouchPoint &tp = pts[0];
    unsigned long duration = tp.touch_end - tp.touch_start;
    float movement = sqrt(pow(tp.x - tp.start_x, 2) + pow(tp.y - tp.start_y, 2));
    
    // Nur TAP/DOUBLE_TAP bei Release (alles andere wird live erkannt)
    if (movement <= 20 && duration <= 250) {
      if (now - last_tap_time < 400 && 
          abs(tp.x - last_tap_x) < 20 && 
          abs(tp.y - last_tap_y) < 20) {
        // DOUBLE TAP
        Serial.printf("GESTE: DOUBLE_TAP at (%d,%d)\n", tp.x, tp.y);
        setGesture(GestureType::DoubleTap, tp.x, tp.y, 2, 1, now);
        last_tap_time = 0;
      } else {
        // SINGLE TAP
        Serial.printf("GESTE: TAP at (%d,%d)\n", tp.x, tp.y);
        setGesture(GestureType::Tap, tp.x, tp.y, 1, 1, now);
        last_tap_time = now;
        last_tap_x = tp.x;
        last_tap_y = tp.y;
      }
    }
  } else if (last_count == 2) {
    // Two-Finger-Tap (nur wenn keine Pinch/Rotate erkannt)
    Serial.printf("GESTE: TWO_FINGER_TAP\n");
    setGesture(GestureType::TwoFingerTap, (pts[0].x + pts[1].x) / 2, (pts[0].y + pts[1].y) / 2, 2, 2, now);
  }
}
