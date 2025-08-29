// ============================================================================
// File: src/comm/RS485Bus.h
// ----------------------------------------------------------------------------
#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#include "../config/pins.h"

// Einfaches, halbduplexes RS485-Modul für MAX3485/compatible Transceiver
// DE und /RE sind meist gebrückt und aktiv HIGH für TX.

#ifndef PIN_RS485_TX
  #define PIN_RS485_TX 15
#endif
#ifndef PIN_RS485_RX
  #define PIN_RS485_RX 14
#endif
#ifndef PIN_RS485_DE
  #define PIN_RS485_DE 16
#endif
#ifndef RS485_UART_PORT
  #define RS485_UART_PORT 1
#endif

class RS485Bus {
public:
  bool begin(uint32_t baud = 115200, uint32_t config = SERIAL_8N1);
  size_t write(const uint8_t* data, size_t len);
  size_t write(const String& s){ return write((const uint8_t*)s.c_str(), s.length()); }
  int available() { return _ser.available(); }
  int read() { return _ser.read(); }
  void flush() { _ser.flush(); }
  void setBaud(uint32_t baud, uint32_t config = SERIAL_8N1);
private:
  void setTxMode(bool on);
  HardwareSerial _ser{RS485_UART_PORT};
};
