// ============================================================================
// File: src/comm/SerialConsole.h
// ----------------------------------------------------------------------------
#pragma once
#include <Arduino.h>
#include <functional>

class SerialConsole {
public:
  using Handler = std::function<void(const String&)>;
  void begin(uint32_t baud = 115200){ Serial.begin(baud); }
  void onLine(Handler h){ _handler = h; }
  void loop(){
    while (Serial.available()){
      char c = (char)Serial.read();
      if (c=='
') continue;
      if (c=='
') { if (_handler) _handler(_buf); _buf = ""; }
      else { _buf += c; if (_buf.length()>512) _buf.remove(0); }
    }
  }
private:
  String _buf; Handler _handler;
};