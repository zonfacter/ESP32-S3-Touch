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

void App::loop(){
  unsigned long now = millis();
  
  // FPS berechnen (unverändert)
  float dt = (now - _lastFrame) / 1000.0f; 
  if (dt < 1e-6f) dt = 1e-6f;
  _fps = 0.9f * _fps + 0.1f * (1.0f / dt); 
  _lastFrame = now;

  // ============================================
  // VEREINFACHTE TOUCH-VERARBEITUNG (geändert)
  // ============================================
  
  // Touch-Daten lesen (IRQ oder Polling)
  bool doRead = CST328Touch::irqFlag; 
  CST328Touch::irqFlag = false;
  if (!doRead && (now % 17 == 0)) doRead = true; // 60fps Polling-Fallback
  
  if (doRead){ 
    if (_touch.readFrame()) { 
      _touch.mapAndTrack(); 
    } 
  }

  // Touch-Points abrufen
  TouchPoint pts[MAX_TOUCH_POINTS]; 
  _touch.getTouchPoints(pts);
  uint8_t activeCount = _touch.activeCount();
  // Nach _touch.getTouchPoints(pts);
if(ac > 0) {
  Serial.printf("Touch-Debug: %d aktiv bei (%d,%d)\n", ac, pts[0].x, pts[0].y);
}
  // ============================================
  // SOFORTIGE GESTEN-VERARBEITUNG (NEU!)
  // ============================================
  
  // Zusätzliche Touch-End Timestamps setzen (falls nicht von CST328Touch gemacht)
  for(int i = 0; i < MAX_TOUCH_POINTS; i++){
    if(pts[i].was_active_last_frame && !pts[i].active){
      pts[i].touch_end = now; // Touch wurde gerade beendet
    }
  }
  
  // ❌ ALTE SETTLE-TIME LOGIK ENTFERNT:
  // static uint8_t lastAc = 0; 
  // static unsigned long acChangedAt = 0;
  // if (ac != lastAc){ lastAc = ac; acChangedAt = now; }
  // bool countStable = (now - acChangedAt) >= TOUCH_SETTLE_MS;
  
  // ✅ NEUE SOFORTIGE GESTEN-VERARBEITUNG:
  GestureEvent g = _gest.process(pts, activeCount);
  
  if (g.type != GestureType::None){
    _lastGesture = g;
    _audio.playGesture(g.type);
    
    // Verbessertes Debug-Output (wie in .ino)
    Serial.printf("🎭 GESTE: ");
    switch (g.type) {
      case GestureType::Tap: Serial.print("TAP"); break;
      case GestureType::DoubleTap: Serial.print("DOUBLE_TAP ✓"); break;
      case GestureType::LongPress: Serial.print("LONG_PRESS"); break;
      case GestureType::SwipeLeft: Serial.print("SWIPE_LEFT"); break;
      case GestureType::SwipeRight: Serial.print("SWIPE_RIGHT"); break;
      case GestureType::SwipeUp: Serial.print("SWIPE_UP"); break;
      case GestureType::SwipeDown: Serial.print("SWIPE_DOWN"); break;
      case GestureType::PinchIn: Serial.print("PINCH_IN"); break;
      case GestureType::PinchOut: Serial.print("PINCH_OUT"); break;
      case GestureType::RotateCW: Serial.print("ROTATE_CW"); break;
      case GestureType::RotateCCW: Serial.print("ROTATE_CCW"); break;
      case GestureType::TwoFingerTap: Serial.print("TWO_FINGER_TAP ✓"); break;
      case GestureType::ThreeFingerTap: Serial.print("THREE_FINGER_TAP ✓"); break;
      default: Serial.print("UNKNOWN"); break;
    }
    Serial.printf(" at (%d,%d) value=%.2f fingers=%d\n", 
                  g.x, g.y, g.value, g.finger_count);
  }

  // ============================================
  // REST BLEIBT UNVERÄNDERT
  // ============================================
  
  // IMU lesen (entspannt 50 Hz)
  static unsigned long lastIMU = 0;
  if (now - lastIMU >= 20){ 
    _imu.read(_imuData); 
    lastIMU = now; 
  }

  // HUD ~30 Hz (jetzt 60fps für flüssiges Touch-Feedback)
  if (now - _lastHUD >= 17){ // 17ms = ~60fps
    _disp.renderHUD(_lastGesture, _fps,
                    _imuData.ax, _imuData.ay, _imuData.az,
                    _imuData.gx, _imuData.gy, _imuData.gz);
    _lastHUD = now;
  }

  // Konsole & RS485 (unverändert)
  _console.loop();

  // RS485 RX → optional echo/Log (unverändert)
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
}