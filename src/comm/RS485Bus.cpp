// ============================================================================
// File: src/comm/RS485Bus.cpp
// ----------------------------------------------------------------------------
#include "RS485Bus.h"

bool RS485Bus::begin(uint32_t baud, uint32_t config){
  pinMode(PIN_RS485_DE, OUTPUT);
  setTxMode(false);
  _ser.begin((unsigned long)baud, (uint32_t)config, PIN_RS485_RX, PIN_RS485_TX);
  return true;
}

void RS485Bus::setBaud(uint32_t baud, uint32_t config){
  _ser.updateBaudRate((unsigned long)baud);
  // Falls Config wechseln soll: neu starten
  _ser.end();
  _ser.begin((unsigned long)baud, (uint32_t)config, PIN_RS485_RX, PIN_RS485_TX);
}

void RS485Bus::setTxMode(bool on){
  digitalWrite(PIN_RS485_DE, on ? HIGH : LOW);
}

size_t RS485Bus::write(const uint8_t* data, size_t len){
  setTxMode(true);
  size_t w = _ser.write(data, len);
  _ser.flush();
  setTxMode(false);
  return w;
}