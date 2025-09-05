// ============================================================================
// File: src/app/App.cpp - ENHANCED DEBUGGING
// ----------------------------------------------------------------------------
#include "App.h"
#include "../config/pins.h"
#include "../config/params.h"

bool App::begin(){
  Serial.begin(115200);
  //Serial.setTxTimeoutMs(0);
  unsigned long t0 = millis();
  while (!Serial && (millis() - t0) < 2000) { delay(10); }
  delay(100); // Mehr Zeit für Serial
  
  Serial.println("\n" + String('=', 60));
  Serial.println("[APP] ESP32-S3 Touch LCD 2.8\" - Debug Version");
  Serial.println("[APP] Start initialization...");
  Serial.println(String('=', 60));

  // I2C Busse mit mehr Debug
  Serial.println("[APP] Setting up I2C buses...");
  Wire.begin(PIN_IMU_SDA, PIN_IMU_SCL, I2C_FREQ_HZ);
  Wire1.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL, I2C_FREQ_HZ);
  delay(100); // Mehr Zeit
  
  Serial.printf("[APP] I2C0 (IMU): SDA=%d SCL=%d @ %dHz\n", 
                PIN_IMU_SDA, PIN_IMU_SCL, I2C_FREQ_HZ);
  Serial.printf("[APP] I2C1 (TOUCH): SDA=%d SCL=%d @ %dHz\n", 
                PIN_TOUCH_SDA, PIN_TOUCH_SCL, I2C_FREQ_HZ);
  
  scanI2C(Wire,  "I2C0(IMU)");
  scanI2C(Wire1, "I2C1(TOUCH)");

  // Display init
  Serial.println("[APP] Display init...");
  if (!_disp.begin()) { 
    Serial.println("[APP] ERROR: Display init failed!"); 
    return false; 
  }
  Serial.println("[APP] Display OK");

  // Touch init mit mehr Debug
  Serial.println("[APP] Touch init...");
  bool touchOK = _touch.begin();
  if (!touchOK) {
    Serial.println("[APP] WARNING: Touch init failed - continuing anyway");
  } else {
    Serial.println("[APP] Touch OK");
  }

  // IMU init mit Debug
  Serial.println("[APP] IMU init...");
  bool imuOK = _imu.begin();
  if (!imuOK) {
    Serial.println("[APP] WARNING: IMU init failed - continuing anyway");
  } else {
    Serial.println("[APP] IMU OK");
  }

  // Audio init
  Serial.println("[APP] Audio init...");
  if (!_audio.begin()) {
    Serial.println("[APP] WARNING: Audio init failed - continuing anyway");
  } else {
    Serial.println("[APP] Audio OK");
  }

  // RS485 & Console
  _rs485.begin(115200);
  _console.begin(115200);
  _console.onLine([this](const String& line){
    if (line.startsWith("rs485send ")){
      String payload = line.substring(10);
      _rs485.write(payload + "\r\n");
      Serial.println("[RS485] TX: " + payload);
    }
    else if (line.startsWith("rs485baud ")){
      uint32_t b = line.substring(10).toInt();
      if (b > 0){
        _rs485.setBaud(b);
        Serial.printf("[RS485] Baud -> %u\n", b);
      }
    }
    else if (line == "rs485echo on"){
      _echo485 = true;  Serial.println("[RS485] Echo ON");
    }
    else if (line == "rs485echo off"){
      _echo485 = false; Serial.println("[RS485] Echo OFF");
    }
    else if (line == "debug touch"){
      Serial.println("[DEBUG] Touch active points: " + String(_touch.activeCount()));
      TouchPoint pts[MAX_TOUCH_POINTS];
      _touch.getTouchPoints(pts);
      for(int i=0; i<MAX_TOUCH_POINTS; i++){
        if(pts[i].active) {
          Serial.printf("[DEBUG] Touch %d: (%d,%d) strength=%d\n", 
                        i, pts[i].x, pts[i].y, pts[i].strength);
        }
      }
    }
    else if (line == "debug imu"){
      Serial.printf("[DEBUG] IMU: ax=%.3f ay=%.3f az=%.3f gx=%.1f gy=%.1f gz=%.1f\n",
                    _imuData.ax, _imuData.ay, _imuData.az, 
                    _imuData.gx, _imuData.gy, _imuData.gz);
      float total_g = sqrt(_imuData.ax*_imuData.ax + _imuData.ay*_imuData.ay + _imuData.az*_imuData.az);
      Serial.printf("[DEBUG] |g| = %.3f (should be ~1.0)\n", total_g);
    }

    
    else {
      Serial.println("Commands: rs485send <text> | rs485baud <n> | rs485echo on|off");
      Serial.println("          debug touch | debug imu");
    }
  });

  _gest.reset();
  _lastGesture.type = GestureType::None;
  _lastFrame = millis();
  
  Serial.println("[APP] ==> INIT COMPLETE <==");
  Serial.println();
  return true;
}

void App::scanI2C(TwoWire& w, const char* name){
  Serial.printf("[SCAN] %s...\n", name);
  bool found = false;
  for (uint8_t a=1; a<127; a++){
    w.beginTransmission(a);
    if (w.endTransmission()==0){
      Serial.printf("  -> FOUND: 0x%02X (%d)\n", a, a);
      found = true;
    }
  }
  if (!found) {
    Serial.println("  -> NO DEVICES FOUND!");
  }
  Serial.flush();
}

void App::loop(){
  unsigned long now = millis();
  
  // FPS berechnen
  float dt = (now - _lastFrame) / 1000.0f; 
  if (dt < 1e-6f) dt = 1e-6f;
  _fps = 0.9f * _fps + 0.1f * (1.0f / dt); 
  _lastFrame = now;

  // TOUCH: Aggressive reading - sowohl IRQ als auch Poll
  bool doRead = false;
  if (CST328Touch::irqFlag) {
    CST328Touch::irqFlag = false;
    doRead = true;
  }
  // Poll alle 5ms zusätzlich
  static unsigned long lastTouchPoll = 0;
  if (now - lastTouchPoll >= 5) {
    doRead = true;
    lastTouchPoll = now;
  }
  
  if (doRead) { 
    if (_touch.readFrame()) { 
      _touch.mapAndTrack(); 
    }
  }

  // Gesten - VEREINFACHT: Weniger Stabilität erforderlich
  TouchPoint pts[MAX_TOUCH_POINTS]; 
  _touch.getTouchPoints(pts);
  uint8_t ac = _touch.activeCount();
  
  static uint8_t lastAc = 0; 
  static unsigned long acChangedAt = 0;
  if (ac != lastAc) { 
    lastAc = ac; 
    acChangedAt = now; 
    Serial.printf("[GESTURE] Touch count changed: %d\n", ac);
  }
  
  // Reduzierte Settle-Zeit
  bool countStable = (now - acChangedAt) >= TOUCH_SETTLE_MS;

  GestureEvent g;
  if (countStable) {
    g = _gest.process(pts, ac);
  } else {
    g.type = GestureType::None;
  }
  
  if (g.type != GestureType::None) {
    _lastGesture = g;
    Serial.printf("[GESTURE] Detected: %d at (%d,%d) value=%.2f\n", 
                  (int)g.type, g.x, g.y, g.value);
    _audio.playGesture(g.type);
  }

  // IMU lesen - häufiger für Debug
  static unsigned long lastIMU = 0;
  if (now - lastIMU >= 50){ // 20Hz
    bool imuSuccess = _imu.read(_imuData); 
    if (!imuSuccess) {
      static int imuFailCount = 0;
      imuFailCount++;
      if (imuFailCount % 100 == 1) { // Log every 100th failure
        Serial.printf("[IMU] Read failures: %d\n", imuFailCount);
      }
    }
    lastIMU = now;
  }

  // HUD ~30 Hz
  if (now - _lastHUD >= 33){
    _disp.renderHUD(_lastGesture, _fps,
                    _imuData.ax, _imuData.ay, _imuData.az,
                    _imuData.gx, _imuData.gy, _imuData.gz);
    
    // Touch-Visualisierung temporär auskommentiert
    // _disp.renderTouchPoints(pts, ac);
    
    _lastHUD = now;
  }

  // Konsole & RS485
  _console.loop();

  // RS485 RX
  static char rxbuf[256]; static size_t rxi = 0;
  while (_rs485.available() > 0){
    int c = _rs485.read(); if (c < 0) break;
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
