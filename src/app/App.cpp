// ============================================================================
// File: src/app/App.cpp
// ----------------------------------------------------------------------------
#include "App.h"
#include "../config/pins.h"
#include "../config/params.h"

bool App::begin(){
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
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
void App::updateMultiTouch(){
  unsigned long now = millis();
  static unsigned long lastPoll = 0;
  
  TouchPoint pts[MAX_TOUCH_POINTS]; 
  _touch.getTouchPoints(pts);
  
for(int i = 0; i < MAX_TOUCH_POINTS; i++){
  if(pts[i].was_active_last_frame && !pts[i].active){
    pts[i].touch_end = now;
    Serial.printf("DEBUG: Touch[%d] ENDED at %lu\n", i, now);
  }
}
  
  // POLLING-TEST statt IRQ (alle 17ms)
  if (now - lastPoll >= 17) {
    bool hasTouch = _touch.readFrame();
    lastPoll = now;
    
    Serial.printf("DEBUG: readFrame()=%s\n", hasTouch ? "true" : "false");
    
    if (!hasTouch) {
      Serial.println("DEBUG: No touch - should release all");
    } else {
      _touch.mapAndTrack();
    }
  } else {
    return; // Kein Touch-Update in diesem Frame
  }
  
  // DEBUG: Touch-Point-Status
  _touch.getTouchPoints(pts);
  uint8_t current_active_count = _touch.activeCount();
  Serial.printf("DEBUG: Active count = %d\n", current_active_count);
  
  for(int i = 0; i < MAX_TOUCH_POINTS; i++){
    if(pts[i].active) {
      Serial.printf("  Point[%d]: (%d,%d) s=%d\n", i, pts[i].x, pts[i].y, pts[i].strength);
    }
  }
  
  // Gesten verarbeiten
  GestureEvent g = _gest.process(pts, current_active_count);
  Serial.printf("DEBUG: Gesture process - last_count=%d, current_count=%d\n", 
              _gest._lastActiveCount, current_active_count);

  if (g.type != GestureType::None){
    _lastGesture = g;
    // _audio.playGesture(g.type); // Weiterhin auskommentiert
    
    Serial.printf("GESTE: ");
    switch (g.type) {
      case GestureType::Tap: Serial.print("TAP"); break;
      case GestureType::DoubleTap: Serial.print("DOUBLE_TAP"); break;
      case GestureType::LongPress: Serial.print("LONG_PRESS"); break;
      case GestureType::SwipeLeft: Serial.print("SWIPE_LEFT"); break;
      case GestureType::SwipeRight: Serial.print("SWIPE_RIGHT"); break;
      case GestureType::SwipeUp: Serial.print("SWIPE_UP"); break;
      case GestureType::SwipeDown: Serial.print("SWIPE_DOWN"); break;
      case GestureType::TwoFingerTap: Serial.print("TWO_FINGER_TAP"); break;
      case GestureType::ThreeFingerTap: Serial.print("THREE_FINGER_TAP"); break;
      default: Serial.print("UNKNOWN"); break;
    }
    Serial.printf(" at (%d,%d) value=%.2f fingers=%d\n", 
                  g.x, g.y, g.value, g.finger_count);
  }
}

