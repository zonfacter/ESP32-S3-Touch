// ============================================================================
// File: src/touch/CST328Touch.cpp - KORRUPTIONS-FIX
// ----------------------------------------------------------------------------
#include "CST328Touch.h"

volatile bool CST328Touch::irqFlag = false;

void IRAM_ATTR CST328Touch::onIntISR(){
  irqFlag = true;
}

bool CST328Touch::begin(){
  Serial.println("[TOUCH] CST328 Init...");
  
  // Reset Touch Controller
  pinMode(PIN_TOUCH_RST, OUTPUT);
  digitalWrite(PIN_TOUCH_RST, LOW); 
  delay(50);
  digitalWrite(PIN_TOUCH_RST, HIGH); 
  delay(100);

  // Interrupt Setup
  pinMode(PIN_TOUCH_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_TOUCH_INT), onIntISR, FALLING);
  
  // Test CST328 Kommunikation
  Wire1.beginTransmission(CST328_I2C_ADDR);
  if (Wire1.endTransmission() != 0) {
    Serial.printf("[TOUCH] ERROR: CST328 not found at 0x%02X\n", CST328_I2C_ADDR);
    return false;
  }
  
  Serial.println("[TOUCH] CST328 found and ready");
  return true;
}

bool CST328Touch::readReg16(uint16_t reg, uint8_t* buf, size_t len){
  Wire1.beginTransmission(CST328_I2C_ADDR);
  Wire1.write((uint8_t)(reg >> 8));
  Wire1.write((uint8_t)(reg & 0xFF));
  if (Wire1.endTransmission(false) != 0) return false;
  
  size_t got = Wire1.requestFrom((int)CST328_I2C_ADDR, (int)len, (int)true);
  if (got != len) return false;
  
  for (size_t i = 0; i < len; i++) {
    if (!Wire1.available()) return false;
    buf[i] = Wire1.read();
  }
  return true;
}

void CST328Touch::resetController() {
  Serial.println("[TOUCH] Resetting CST328 due to corruption...");
  
  // Hardware Reset
  digitalWrite(PIN_TOUCH_RST, LOW);
  delay(10);
  digitalWrite(PIN_TOUCH_RST, HIGH);
  delay(50);
  
  // Reset internal state
  for (auto &p : _points) {
    p.active = false;
    p.was_active_last_frame = false;
  }
  _activeCount = 0;
  _rawCount = 0;
  _corruptionCount = 0;
  
  Serial.println("[TOUCH] Reset complete");
}

bool CST328Touch::readFrame(){
  // Anzahl aktive Punkte lesen
  uint8_t nbuf[1];
  if (!readReg16(CST328_REG_NUM, nbuf, 1)) {
    _corruptionCount++;
    if (_corruptionCount > 5) {
      resetController();
      return false;
    }
    return false;
  }
  
  _rawCount = nbuf[0] & 0x0F;
  if (_rawCount > MAX_TOUCH_POINTS) _rawCount = MAX_TOUCH_POINTS;

  // Alle Punkte inaktiv setzen wenn keine Touch-Punkte
  if (_rawCount == 0) {
    for (auto &p : _points) {
      if (p.active) {
        Serial.printf("[TOUCH] RELEASE touch %d\n", findActiveIndex(&p));
      }
      p.active = false;
      p.was_active_last_frame = false;
    }
    _activeCount = 0;
    _corruptionCount = 0; // Reset corruption counter on clean read
    return true;
  }

  // Touch-Daten lesen (6 Bytes pro Punkt)
  uint8_t buf[6 * MAX_TOUCH_POINTS] = {0};
  if (!readReg16(CST328_REG_COORD, buf, _rawCount * 6)) {
    _corruptionCount++;
    if (_corruptionCount > 5) {
      resetController();
    }
    return false;
  }

  // Korruptions-Detection auf Paket-Level
  bool packetCorrupted = false;
  for (uint8_t i = 0; i < _rawCount * 6; i++) {
    // Prüfe auf unrealistic patterns (alle Bytes gleich, etc.)
    if (i >= 6) {
      // Wiederholendes Pattern detection
      bool isRepeating = true;
      for (uint8_t j = 0; j < 6; j++) {
        if (buf[j] != buf[i - 6 + (j % 6)]) {
          isRepeating = false;
          break;
        }
      }
      if (isRepeating) {
        packetCorrupted = true;
        break;
      }
    }
  }
  
  if (packetCorrupted) {
    _corruptionCount++;
    Serial.printf("[TOUCH] Packet corruption detected (count: %d)\n", _corruptionCount);
    
    if (_corruptionCount > 3) { // Weniger tolerant
      resetController();
    }
    return false;
  }

  int validPoints = 0;
  
  // Raw-Punkte parsen mit verstärkter Validierung
  for (uint8_t i = 0; i < _rawCount; i++) {
    uint8_t* b = &buf[i * 6];
    
    // HIGH-LOW Byte-Order (bestätigt)
    uint16_t x = ((uint16_t)b[0] << 8) | b[1];
    uint16_t y = ((uint16_t)b[2] << 8) | b[3];
    uint16_t strength = ((uint16_t)b[4] << 8) | b[5];
    
    // VERSTÄRKTE Validierung
    bool valid = true;
    
    // 1. Basis-Koordinaten-Check
    if (x > 4096 || y > 4096) {
      valid = false;
    }
    
    // 2. Strength-Check
    if (strength > 20000 || strength < 5) {
      valid = false;  
    }
    
    // 3. Sprung-Detection mit dynamischen Schwellwerten
    static uint16_t lastValidX = 0, lastValidY = 0;
    static unsigned long lastValidTime = 0;
    unsigned long now = millis();
    
    if (lastValidX > 0 && lastValidY > 0 && (now - lastValidTime) < 1000) {
      uint16_t dx = (x > lastValidX) ? (x - lastValidX) : (lastValidX - x);
      uint16_t dy = (y > lastValidY) ? (y - lastValidY) : (lastValidY - y);
      
      // Dynamische Schwellwerte basierend auf Zeit zwischen Punkten
      uint16_t maxJump = (now - lastValidTime < 50) ? 200 : 500;
      
      if (dx > maxJump || dy > maxJump) {
        valid = false;
      }
    }
    
    // 4. Check für "stuck" Werte (gleiche Koordinaten zu oft)
    static uint16_t stuckX = 0, stuckY = 0;
    static int stuckCount = 0;
    
    if (x == stuckX && y == stuckY) {
      stuckCount++;
      if (stuckCount > 10) { // Nach 10 gleichen Werten = stuck
        valid = false;
      }
    } else {
      stuckX = x; stuckY = y; stuckCount = 0;
    }
    
    if (!valid) {
      _corruptionCount++;
      
      // Nur jede 10. invalide Meldung loggen (spam-Schutz)
      static int invalidLogCount = 0;
      if (++invalidLogCount % 10 == 1) {
        Serial.printf("[TOUCH] Invalid #%d: raw(%d,%d) s=%d dx=%d dy=%d stuck=%d\n", 
                      invalidLogCount, x, y, strength, 
                      lastValidX > 0 ? abs((int)x - (int)lastValidX) : 0,
                      lastValidY > 0 ? abs((int)y - (int)lastValidY) : 0,
                      stuckCount);
      }
      
      // Bei zu vielen invaliden Werten -> Reset
      if (_corruptionCount > 8) {
        resetController();
        return false;
      }
      
      continue; // Skip diesen Punkt
    }
    
    // Reset corruption counter bei gültigen Daten
    _corruptionCount = 0;
    
    // Update für Sprung-Detection
    lastValidX = x; lastValidY = y; lastValidTime = now;
    
    _raw[validPoints] = {x, y, strength};
    
    // Koordinaten-Transformation
    uint16_t dispX, dispY;
    rawToDisplay(x, y, dispX, dispY);
    
    // Touch-Punkt aktualisieren
    _points[validPoints].x = dispX;
    _points[validPoints].y = dispY;
    _points[validPoints].strength = strength;
    _points[validPoints].active = true;
    
    if (!_points[validPoints].was_active_last_frame) {
      _points[validPoints].touch_start = millis();
      _points[validPoints].start_x = dispX;
      _points[validPoints].start_y = dispY;
      Serial.printf("[TOUCH] NEW valid touch %d at (%d,%d) from raw(%d,%d)\n", 
                    validPoints, dispX, dispY, x, y);
    }
    _points[validPoints].was_active_last_frame = true;
    validPoints++;
  }
  
  // Restliche Slots inaktiv
  for (uint8_t i = validPoints; i < MAX_TOUCH_POINTS; i++) {
    if (_points[i].active) {
      Serial.printf("[TOUCH] RELEASE touch %d\n", i);
    }
    _points[i].active = false;
    _points[i].was_active_last_frame = false;
  }
  
  _activeCount = validPoints;
  return true;
}

int CST328Touch::findActiveIndex(const TouchPoint* p) const {
  for (int i = 0; i < MAX_TOUCH_POINTS; i++) {
    if (&_points[i] == p) return i;
  }
  return -1;
}

void CST328Touch::rawToDisplay(uint16_t rx, uint16_t ry, uint16_t& dx, uint16_t& dy) const {
  // Normalisierung mit korrigierter Raw-Range
  float nx = (float)(rx - TOUCH_RAW_X_MIN) / (float)(TOUCH_RAW_X_MAX - TOUCH_RAW_X_MIN);
  float ny = (float)(ry - TOUCH_RAW_Y_MIN) / (float)(TOUCH_RAW_Y_MAX - TOUCH_RAW_Y_MIN);
  nx = constrain(nx, 0.f, 1.f);
  ny = constrain(ny, 0.f, 1.f);

  // Achsentausch & Invertierung  
  float ax = TOUCH_SWAP_XY ? ny : nx;
  float ay = TOUCH_SWAP_XY ? nx : ny;

  if (TOUCH_INVERT_X) ax = 1.f - ax;
  if (TOUCH_INVERT_Y) ay = 1.f - ay;

  // Skalierung auf Display
  dx = (uint16_t)lroundf(ax * (DISPLAY_WIDTH - 1));
  dy = (uint16_t)lroundf(ay * (DISPLAY_HEIGHT - 1));
}

void CST328Touch::mapAndTrack() {
  // Alles wird in readFrame() gemacht
}

void CST328Touch::getTouchPoints(TouchPoint out[MAX_TOUCH_POINTS]) const {
  memcpy(out, _points, sizeof(_points));
}