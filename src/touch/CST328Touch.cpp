// ============================================================================
// File: src/touch/CST328Touch.cpp - KORRUPTIONS-FIX
// ----------------------------------------------------------------------------
#include "CST328Touch.h"

#include "../config/params.h"   // <-- wichtig: bringt TOUCH_MIN_STRENGTH, DISPLAY_WIDTH/HEIGHT

// Fallback (falls eine ältere params.h das noch nicht definiert):
#ifndef TOUCH_MIN_STRENGTH
#define TOUCH_MIN_STRENGTH 0
#endif

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

// NEU: writeReg16 Implementierung
bool CST328Touch::writeReg16(uint16_t reg, const uint8_t* buf, size_t len){
  Wire1.beginTransmission(CST328_I2C_ADDR);
  Wire1.write((uint8_t)(reg >> 8));
  Wire1.write((uint8_t)(reg & 0xFF));
  
  for (size_t i = 0; i < len; i++) {
    Wire1.write(buf[i]);
  }
  
  return (Wire1.endTransmission() == 0);
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

// ============================================================================
// CST328Touch::readFrame() – Parsing exakt nach CST328-Datenblatt v2.2
//  • Liest D000..D01A (27 Bytes) in einem Rutsch
//  • Fingerzahl: D005 (low nibble), Signatur: D006 == 0xAB
//  • Pro Finger 5 Bytes: [ID/Status][X_H][Y_H][XY low nibbles][Pressure]
//  • 12-bit: X=(XH<<4)|(XY>>4), Y=(YH<<4)|(XY&0x0F)
//  • Status-Nibble: konservativ „!=0“ als aktiv (Touch), um FW-Varianten abzudecken
//  • Setzt Releases bei 0 Fingern oder wenn nach Filter 0 übrig
//  • Sanftes Debug (throttled) zeigt D005/D006/kept-Punkte
// ============================================================================
bool CST328Touch::readFrame()
{
  // 1) Gesamten Block D000..D01A holen (27 Bytes)
  uint8_t buf[27] = {0};
  if (!readReg16(0xD000, buf, sizeof(buf))) {
    return false;
  }

  // 2) Fingerzahl + Signatur prüfen
  const uint8_t d005 = buf[0x05];               // Fingerzahl im low nibble
  const uint8_t d006 = buf[0x06];               // soll 0xAB sein
  uint8_t reported  = (d005 & 0x0F);
  uint8_t count     = (reported > MAX_TOUCH_POINTS) ? MAX_TOUCH_POINTS : reported;

  // Optional: Modus/Signatur checken – wenn nicht 0xAB, (einmal) Normalmodus setzen
  static bool triedNormal = false;
  if (d006 != 0xAB && !triedNormal) {
    triedNormal = true;
    // 0xD109 = ENUM_MODE_NORMAL (laut DB), Wert ist i.d.R. egal (0x00 reicht)
    uint8_t val = 0x00;
    // Falls du writeReg16 noch nicht hast: gleiche Signatur wie readReg16, nur Schreibpfad.
    (void)writeReg16(0xD109, &val, 1);          // in Normalmodus schalten
    return false;                                // nächster Frame wird korrekt
  }

  // 3) 0 Finger → Releases und zurück
  if (count == 0) {
    for (auto &p : _points) {
      if (p.active) {
        p.active = false;
        p.touch_end = millis();
        p.was_active_last_frame = true;
      }
    }
    _rawCount = 0;
    _activeCount = 0;

    // Debug (throttled)
    static unsigned long t0 = 0;
    if (millis() - t0 > 250) {
      Serial.printf("[TOUCH] D005=0x%02X D006=0x%02X count=0\n", d005, d006);
      t0 = millis();
    }
    return true;
  }

  // 4) Offsets der Fingerslots gemäß Tabelle
  auto baseOf = [](uint8_t idx)->uint8_t {
    switch (idx) {
      case 0: return 0x00; // F1: D000..D004
      case 1: return 0x07; // F2: D007..D00B
      case 2: return 0x0C; // F3: D00C..D010
      case 3: return 0x11; // F4: D011..D015
      case 4: return 0x16; // F5: D016..D01A
      default: return 0x00;
    }
  };

  // 5) Punkte dekodieren
  uint8_t kept = 0;
  for (uint8_t i = 0; i < count; ++i) {
    const uint8_t off    = baseOf(i);
    const uint8_t idstat = buf[off + 0];    // high nibble: ID, low: Status
    const uint8_t xh     = buf[off + 1];    // X >> 4
    const uint8_t yh     = buf[off + 2];    // Y >> 4
    const uint8_t xy     = buf[off + 3];    // [X low4 | Y low4]
    const uint8_t pres   = buf[off + 4];    // 8-bit pressure

    const uint8_t status = (idstat & 0x0F);

    // Konservativ: alles != 0 als „aktiv“ akzeptieren.
    // (DB nennt 0x06=Touch; FW-Varianten melden teils 0x07/0x0F u.ä.)
    if (status != 0x00) {
      const uint16_t x = ((uint16_t)xh << 4) | (xy >> 4);
      const uint16_t y = ((uint16_t)yh << 4) | (xy & 0x0F);
      const uint16_t s = pres; // 8-bit Druck
      _raw[kept++] = { x, y, s };
    }
  }
  _rawCount = kept;

  // 6) Wenn nach Status-Filter nichts übrig → Releases
  if (_rawCount == 0) {
    for (auto &p : _points) {
      if (p.active) {
        p.active = false;
        p.touch_end = millis();
        p.was_active_last_frame = true;
      }
    }
    _activeCount = 0;
  }

  // 7) Debug-Ausgabe (throttled)
  static unsigned long lastDbg = 0;
  if (millis() - lastDbg > 150) {
    Serial.printf("[TOUCH] D005=0x%02X D006=0x%02X reported=%u kept=%u  ",
                  d005, d006, reported, _rawCount);
    for (uint8_t i=0; i<_rawCount; ++i) {
      Serial.printf("#%u raw(%u,%u) s=%u  ", i, _raw[i].x, _raw[i].y, _raw[i].strength);
    }
    Serial.println();
    lastDbg = millis();
  }

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