// ============================================================================
// CST328Touch.cpp - DIREKT AUS FUNKTIONIERENDER .INO ADAPTIERT
// ============================================================================

#include "CST328Touch.h"

volatile bool CST328Touch::irqFlag = false;

void IRAM_ATTR CST328Touch::onIntISR(){
  irqFlag = true;
}

bool CST328Touch::begin(){
  Wire1.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL, I2C_FREQ_HZ);

  // GENAU WIE IN .INO: Reset-Prozedur
  pinMode(PIN_TOUCH_RST, OUTPUT);
  digitalWrite(PIN_TOUCH_RST, HIGH);
  delay(50);
  digitalWrite(PIN_TOUCH_RST, LOW);
  delay(5);
  digitalWrite(PIN_TOUCH_RST, HIGH);
  delay(50);

  pinMode(PIN_TOUCH_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_TOUCH_INT), onIntISR, FALLING);

  // .INO-Style Config-Read
  uint8_t buf[24];
  if (!readReg16(0xD101, buf, 0)) return false; // DEBUG_INFO_MODE
  
  if (readReg16(0xD1F4, buf, 24)) { // DEBUG_INFO_TP_NTX
    uint16_t verification = (((uint16_t)buf[11] << 8) | buf[10]);
    readReg16(0xD109, buf, 0); // NORMAL_MODE
    if (verification == 0xCACA) {
      Serial.println("CST328 .ino-Style Init OK!");
      return true;
    }
  }
  
  return false;
}


// ============================================================================
// CST328Touch.cpp - EXAKTE .ino-Konfiguration adaptiert
// ============================================================================

// LESEN: 16-bit Registeradresse -> len Bytes in buf
bool CST328Touch::readReg16(uint16_t reg, uint8_t* buf, size_t len) {
  if (!buf || len == 0) return false;

  Wire1.beginTransmission(CST328_I2C_ADDR);
  Wire1.write((uint8_t)(reg >> 8));
  Wire1.write((uint8_t)(reg & 0xFF));
  // Repeated-Start: kein STOP zwischen Addr-Writes und dem Read
  if (Wire1.endTransmission(false) != 0) return false;

  // Letzter Parameter = STOP senden (true)
  size_t got = Wire1.requestFrom((uint8_t)CST328_I2C_ADDR, (uint8_t)len, (uint8_t)true);
  if (got != len) {
    while (Wire1.available()) (void)Wire1.read(); // Puffer leeren
    return false;
  }
  for (size_t i = 0; i < len; ++i) {
    if (!Wire1.available()) return false;
    buf[i] = Wire1.read();
  }
  return true;
}

// SCHREIBEN: 16-bit Registeradresse -> len Datenbytes
bool CST328Touch::writeReg16(uint16_t reg, const uint8_t* data, size_t len) {
  if (!data || len == 0) return false;

  Wire1.beginTransmission(CST328_I2C_ADDR);
  Wire1.write((uint8_t)(reg >> 8));
  Wire1.write((uint8_t)(reg & 0xFF));
  for (size_t i = 0; i < len; ++i) Wire1.write(data[i]);
  return Wire1.endTransmission(true) == 0; // STOP senden
}

// KORRIGIERTE readFrame() mit sauberer Clear-Logik
bool CST328Touch::readFrame(){
  uint8_t buf[41];
  
  // SCHRITT 1: Number Register lesen
  if (!readReg16(0xD005, buf, 1)) {
    return false;
  }
  
  uint8_t touch_cnt = buf[0] & 0x0F;
  Serial.printf("RAW REG: 0x%02X -> touch_cnt=%d\n", buf[0], touch_cnt);
  
  // SCHRITT 2: Bei 0 Punkten - SAUBERES CLEARING
  if (touch_cnt == 0) {
    Serial.println("SHOULD RETURN FALSE!");
    
    // SAUBER: 1 Byte 0x00 an Number Register schreiben
    uint8_t clear_byte = 0x00;
    writeReg16(0xD005, &clear_byte, 1);
    
    // Touch points deaktivieren
    for (auto &p : _points){
      if (p.active){ 
        p.active = false; 
        p.touch_end = millis(); 
        p.long_press_fired = false; 
      }
    }
    _activeCount = 0;
    return false;
  }
  
  // SCHRITT 3: Overflow check
  if (touch_cnt > MAX_TOUCH_POINTS) {
    uint8_t clear_byte = 0x00;
    writeReg16(0xD005, &clear_byte, 1);
    return false;
  }
  
  // SCHRITT 4: Koordinaten lesen
  if (!readReg16(0xD000, &buf[1], 27)) {
    return false;
  }
  
  // SCHRITT 5: Number Register clearen
  uint8_t clear_byte = 0x00;
  writeReg16(0xD005, &clear_byte, 1);
  
  // SCHRITT 6: Data-Extraktion
  _rawCount = touch_cnt;
  for (int i = 0; i < touch_cnt; i++) {
    int num = (i > 0) ? 2 : 0;
    
    uint16_t x = (uint16_t)(((uint16_t)buf[(i * 5) + 2 + num] << 4) + 
                           ((buf[(i * 5) + 4 + num] & 0xF0) >> 4));
    uint16_t y = (uint16_t)(((uint16_t)buf[(i * 5) + 3 + num] << 4) + 
                           (buf[(i * 5) + 4 + num] & 0x0F));
    uint16_t s = ((uint16_t)buf[(i * 5) + 5 + num]);
    
    _raw[i] = {x, y, s};
  }
  
  return true;
}


// NEUE METHODE: Kompletter Controller-Reset
void CST328Touch::forceCompleteReset() {
  Serial.println("FORCE COMPLETE RESET...");
  
  // Interrupt deaktivieren
  detachInterrupt(digitalPinToInterrupt(PIN_TOUCH_INT));
  
  // Längerer Hardware-Reset
  digitalWrite(PIN_TOUCH_RST, HIGH);
  delay(100);
  digitalWrite(PIN_TOUCH_RST, LOW);
  delay(200);  // Viel länger LOW
  digitalWrite(PIN_TOUCH_RST, HIGH);
  delay(300);  // Viel länger HIGH
  
  // I2C Bus komplett neu starten
  Wire1.end();
  delay(100);
  Wire1.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL, I2C_FREQ_HZ);
  delay(100);
  
  // Alle Register aggressiv clearen
  uint8_t clear = 0;
  for (uint16_t reg = 0xD000; reg <= 0xD020; reg++) {
    Wire1.beginTransmission(CST328_I2C_ADDR);
    Wire1.write((uint8_t)(reg >> 8));
    Wire1.write((uint8_t)(reg & 0xFF));
    Wire1.write(clear);
    Wire1.endTransmission(true);
    delay(2);
  }
  
  // Controller re-initialisieren
  uint8_t buf[24];
  readReg16(0xD101, buf, 0); // DEBUG_INFO_MODE
  delay(50);
  readReg16(0xD109, buf, 0); // NORMAL_MODE
  delay(50);
  
  // Interrupt wieder aktivieren
  pinMode(PIN_TOUCH_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_TOUCH_INT), onIntISR, FALLING);
  
  // Alle internen Zustände zurücksetzen
  for (auto &p : _points){
    p.active = false;
    p.was_active_last_frame = false;
    p.touch_end = millis();
  }
  _activeCount = 0;
  _rawCount = 0;
  
  Serial.println("COMPLETE RESET DONE");
}
// VEREINFACHTE Touch-Assignment wie .ino (KEIN Proximity-Tracking)
void CST328Touch::assignByProximity(){
  unsigned long now = millis();
  
  // Previous state speichern
  for (int j = 0; j < MAX_TOUCH_POINTS; j++) {
    _points[j].was_active_last_frame = _points[j].active;
  }
  
  // DIREKTE 1:1-ZUORDNUNG wie .ino - KEINE komplexe Logik
  for (int i = 0; i < MAX_TOUCH_POINTS; i++) {
    if (i < _rawCount) {
      // Touch-Punkt aktiv - DIREKTE Verarbeitung
      uint16_t dx, dy;
      rawToDisplay(_raw[i].x, _raw[i].y, dx, dy);
      
      if (!_points[i].active) {
        // Neuer Touch
        _points[i].touch_start = now;
        _points[i].start_x = dx;
        _points[i].start_y = dy;
        _points[i].long_press_fired = false;
      }
      
      _points[i].x = dx;
      _points[i].y = dy;
      _points[i].strength = _raw[i].strength;
      _points[i].active = true;
      
    } else {
      // Touch-Punkt inaktiv
      if (_points[i].active) {
        _points[i].active = false;
        _points[i].touch_end = now;
        _points[i].long_press_fired = false;
      }
    }
  }
  
  // Active count neu berechnen
  _activeCount = 0; 
  for (auto &p : _points) {
    if (p.active) _activeCount++;
  }
}

// .INO-STYLE MAPPING
void CST328Touch::rawToDisplay(uint16_t rx, uint16_t ry, uint16_t& dx, uint16_t& dy) const{
  uint16_t mappedX, mappedY;
  
  // EXAKT WIE .INO: map() Funktion verwenden
  if (TOUCH_SWAP_XY) {
    mappedX = map(ry, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, 0, DISPLAY_WIDTH - 1);
    mappedY = map(rx, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, 0, DISPLAY_HEIGHT - 1);
  } else {
    mappedX = map(rx, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, 0, DISPLAY_WIDTH - 1);
    mappedY = map(ry, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, 0, DISPLAY_HEIGHT - 1);
  }
  
  if (TOUCH_INVERT_X) {
    mappedX = (DISPLAY_WIDTH - 1) - mappedX;
  }
  
  if (TOUCH_INVERT_Y) {
    mappedY = (DISPLAY_HEIGHT - 1) - mappedY;
  }
  
  dx = constrain(mappedX, 0, DISPLAY_WIDTH - 1);
  dy = constrain(mappedY, 0, DISPLAY_HEIGHT - 1);
}

void CST328Touch::mapAndTrack(){
  assignByProximity();
}

void CST328Touch::getTouchPoints(TouchPoint out[MAX_TOUCH_POINTS]) const{
  memcpy(out, _points, sizeof(_points));
}