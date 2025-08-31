// ============================================================================
// File: src/app/App.h
// ----------------------------------------------------------------------------
#pragma once
#include <Arduino.h>
#include "../display/DisplayManager.h"
#include "../touch/CST328Touch.h"
#include "../gestures/GestureEngine.h"
#include "../audio/AudioI2S.h"
#include "../imu/QMI8658.h"
#include "../comm/RS485Bus.h"
#include "../comm/SerialConsole.h"
RS485Bus _rs485;
SerialConsole _console;
bool _echo485 = false;

class App {
public:
  bool begin();
  void loop();
private:
  void scanI2C(TwoWire& w, const char* name);

  DisplayManager _disp;
  CST328Touch _touch;
  GestureEngine _gest;
  AudioI2S _audio;
  QMI8658 _imu;

  unsigned long _lastHUD = 0;
  unsigned long _lastFrame = 0; float _fps = 0.f;
  GestureEvent _lastGesture;
  IMUData _imuData {0,0,0,0,0,0};
};
