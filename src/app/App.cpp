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
  // FPS berechnen
  float dt = (now - _lastFrame) / 1000.0f; if (dt < 1e-6f) dt = 1e-6f;
  _fps = 0.9f * _fps + 0.1f * (1.0f / dt); _lastFrame = now;

  // TOUCH: per IRQ oder Polling
  bool doRead = CST328Touch::irqFlag; CST328Touch::irqFlag = false;
  if (!doRead && (now % 10 == 0)) doRead = true; // leichter Poll-Fallback
  if (doRead){ if (_touch.readFrame()) { _touch.mapAndTrack(); } }

  // Gesten verarbeiten
  TouchPoint pts[MAX_TOUCH_POINTS]; _touch.getTouchPoints(pts);
  uint8_t ac = _touch.activeCount();
  static uint8_t lastAc = 0; 
static unsigned long acChangedAt = 0;
if (ac != lastAc){ lastAc = ac; acChangedAt = now; }
bool countStable = (now - acChangedAt) >= TOUCH_SETTLE_MS;

GestureEvent g;
if (countStable){
  g = _gest.process(pts, ac);
} else {
  g.type = GestureType::None;
}
if (g.type != GestureType::None){
  _lastGesture = g;
  _audio.playGesture(g.type);   // Audio hat zusätzlich Gate, aber jetzt kommt nur 1 Event
}

  // IMU lesen (entspannt 50 Hz)
  static unsigned long lastIMU = 0;
  if (now - lastIMU >= 20){ _imu.read(_imuData); lastIMU = now; }

  // HUD ~30 Hz
  if (now - _lastHUD >= 33){
    _disp.renderHUD(_lastGesture, _fps,
                    _imuData.ax, _imuData.ay, _imuData.az,
                    _imuData.gx, _imuData.gy, _imuData.gz);
    _lastHUD = now;
  }

  // Konsole & RS485
  _console.loop();

  // RS485 RX → optional echo/Log
  static char rxbuf[256]; static size_t rxi = 0;
  while (_rs485.available() > 0){
    int c = _rs485.read(); if (c < 0) break;
    if (c == '\r') continue;               // CR ignorieren
    if (rxi < sizeof(rxbuf) - 1) rxbuf[rxi++] = (char)c;
    if (c == '\n'){                        // ← korrektes Zeilenende
      rxbuf[rxi] = 0;
      String line = String(rxbuf);
      if (_echo485) _rs485.write(line);    // loopback
      Serial.print("[RS485] RX: "); Serial.print(line);
      rxi = 0;
    }
  }
}