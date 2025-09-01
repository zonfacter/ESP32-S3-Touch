// ============================================================================
// File: src/touch/CST328Touch.cpp
// ----------------------------------------------------------------------------
#include "CST328Touch.h"

volatile bool CST328Touch::irqFlag = false;

void IRAM_ATTR CST328Touch::onIntISR(){
  irqFlag = true;
}

bool CST328Touch::begin(){
  // ENTFERNT: Wire1.begin() - wird bereits in App.cpp initialisiert
  // Wire1.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL, I2C_FREQ_HZ);

  pinMode(PIN_TOUCH_RST, OUTPUT);
  digitalWrite(PIN_TOUCH_RST, LOW); delay(10);
  digitalWrite(PIN_TOUCH_RST, HIGH); delay(50);

  pinMode(PIN_TOUCH_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_TOUCH_INT), onIntISR, FALLING);
  return true;
}

bool CST328Touch::readReg16(uint16_t reg, uint8_t* buf, size_t len){
  Wire1.beginTransmission(CST328_I2C_ADDR);
  Wire1.write((uint8_t)(reg >> 8));
  Wire1.write((uint8_t)(reg & 0xFF));
  if (Wire1.endTransmission(false) != 0) return false; // Repeated Start
  size_t got = Wire1.requestFrom((int)CST328_I2C_ADDR, (int)len, (int)true);
  if (got != len) return false;
  for (size_t i=0;i<len;i++){
    if (!Wire1.available()) return false;
    buf[i] = Wire1.read();
  }
  return true;
}

bool CST328Touch::readFrame(){
  // Anzahl Punkte
  uint8_t nbuf[1];
  if (!readReg16(CST328_REG_NUM, nbuf, 1)) return false;
  _rawCount = nbuf[0] & 0x0F;
  if (_rawCount > MAX_TOUCH_POINTS) _rawCount = MAX_TOUCH_POINTS;

  if (_rawCount == 0){
    for (auto &p : _points){
      if (p.active){ p.active=false; p.touch_end=millis(); p.long_press_fired=false; }
    }
    _activeCount = 0;
    return true;
  }

  uint8_t buf[6*MAX_TOUCH_POINTS] = {0};
  if (!readReg16(CST328_REG_COORD, buf, _rawCount*6)) return false;

  for (uint8_t i=0;i<_rawCount;i++){
    uint8_t* b = &buf[i*6];
    uint16_t x = ((uint16_t)b[0] << 8) | b[1];
    uint16_t y = ((uint16_t)b[2] << 8) | b[3];
    uint16_t s = ((uint16_t)b[4] << 8) | b[5];
    _raw[i] = {x,y,s};
  }

  // *** NEU: Stärke-Filter ***
  uint8_t w = 0;
  for (uint8_t i = 0; i < _rawCount; ++i) {
    if (_raw[i].strength >= TOUCH_MIN_STRENGTH) {
      _raw[w++] = _raw[i];
    }
  }
  _rawCount = w;

  // *** NEU: Release wenn nach Filter keine Punkte mehr da sind ***
  if (_rawCount == 0){
    for (auto &p : _points){
      if (p.active){ p.active=false; p.touch_end=millis(); p.long_press_fired=false; }
    }
    _activeCount = 0;
    return true;
  }

  return true;
}

void CST328Touch::rawToDisplay(uint16_t rx, uint16_t ry, uint16_t& dx, uint16_t& dy) const{
  // normieren 0..1
  float nx = (float)(rx - TOUCH_RAW_X_MIN) / (float)(TOUCH_RAW_X_MAX - TOUCH_RAW_X_MIN);
  float ny = (float)(ry - TOUCH_RAW_Y_MIN) / (float)(TOUCH_RAW_Y_MAX - TOUCH_RAW_Y_MIN);
  nx = constrain(nx, 0.f, 1.f);
  ny = constrain(ny, 0.f, 1.f);

  // Achsentausch
  float ax = TOUCH_SWAP_XY ? ny : nx;
  float ay = TOUCH_SWAP_XY ? nx : ny;

  // Invertierung
  if (TOUCH_INVERT_X) ax = 1.f - ax;
  if (TOUCH_INVERT_Y) ay = 1.f - ay;

  // Skalierung auf Display
  dx = (uint16_t)lroundf(ax * (DISPLAY_WIDTH - 1));
  dy = (uint16_t)lroundf(ay * (DISPLAY_HEIGHT - 1));
}

void CST328Touch::assignByProximity(){
  TouchPoint newPts[MAX_TOUCH_POINTS]{};
  bool usedPrev[MAX_TOUCH_POINTS]{};

  // vorherigen Zustand merken
  for (int j=0;j<MAX_TOUCH_POINTS;j++) newPts[j] = _points[j];
  for (int j=0;j<MAX_TOUCH_POINTS;j++) newPts[j].was_active_last_frame = _points[j].active;

  // neue Rohpunkte den Slots zuweisen
  unsigned long now = millis();
  for (uint8_t i=0;i<_rawCount;i++){
    uint16_t dx, dy; rawToDisplay(_raw[i].x, _raw[i].y, dx, dy);

    int best = -1; float bestD2 = 1e9f;
    for (int j=0;j<MAX_TOUCH_POINTS;j++){
      if (!newPts[j].was_active_last_frame || usedPrev[j]) continue;
      float ddx = (float)dx - (float)newPts[j].x;
      float ddy = (float)dy - (float)newPts[j].y;
      float d2 = ddx*ddx + ddy*ddy;
      if (d2 < bestD2){ bestD2 = d2; best = j; }
    }
    int slot = (best >= 0 && sqrtf(bestD2) < PROXIMITY_TOLER_PX) ? best : i;
    usedPrev[slot] = true;

    if (!newPts[slot].active){
      newPts[slot].touch_start = now;
      newPts[slot].start_x = dx;
      newPts[slot].start_y = dy;
      newPts[slot].long_press_fired = false;
    }
    newPts[slot].x = dx;
    newPts[slot].y = dy;
    newPts[slot].strength = _raw[i].strength;
    newPts[slot].active = true;
  }

  // nicht gematchte alte aktive → released
  for (int j=0;j<MAX_TOUCH_POINTS;j++){
    if (newPts[j].was_active_last_frame && !usedPrev[j] && newPts[j].active){
      // bleibt aktiv → nichts
    } else if (_points[j].active && !usedPrev[j]){
      newPts[j].active = false;
      newPts[j].touch_end = now;
      newPts[j].long_press_fired = false;
    }
  }

  memcpy(_points, newPts, sizeof(_points));
  _activeCount = 0; for (auto &p: _points) if (p.active) _activeCount++;
}

void CST328Touch::mapAndTrack(){
  assignByProximity();
}

void CST328Touch::getTouchPoints(TouchPoint out[MAX_TOUCH_POINTS]) const{
  memcpy(out, _points, sizeof(_points));
}