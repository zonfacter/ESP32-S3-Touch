// ============================================================================
// File: src/display/DisplayManager.h
// ----------------------------------------------------------------------------
#pragma once
#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "../config/pins.h"
#include "../config/params.h"
#include "../core/types.h"

class LGFX_ST7789 : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;
public:
  LGFX_ST7789() {
    {
      auto cfg = _bus.config();
      cfg.spi_host    = SPI3_HOST;
      cfg.spi_mode    = 0;
      cfg.freq_write  = 40000000;
      cfg.freq_read   = 16000000;
      cfg.spi_3wire   = false;
      cfg.use_lock    = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk    = PIN_LCD_SCLK;
      cfg.pin_mosi    = PIN_LCD_MOSI;
      cfg.pin_miso    = PIN_LCD_MISO;
      cfg.pin_dc      = PIN_LCD_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs         = PIN_LCD_CS;
      cfg.pin_rst        = PIN_LCD_RST;
      cfg.pin_busy       = -1;
      cfg.memory_width   = 240;
      cfg.memory_height  = 320;
      cfg.panel_width    = 240;
      cfg.panel_height   = 320;
      cfg.offset_x       = 0;
      cfg.offset_y       = 0;
      cfg.offset_rotation= 0;
      cfg.dummy_read_pixel= 8;
      cfg.dummy_read_bits = 1;
      cfg.readable       = false;
      cfg.invert         = true;
      cfg.rgb_order      = false;
      cfg.dlen_16bit     = false;
      cfg.bus_shared     = true;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};

class DisplayManager {
public:
  bool begin();
  void renderHUD(const GestureEvent& lastGesture, float fps,
                 float ax, float ay, float az, float gx, float gy, float gz);
  LGFX_ST7789& gfx() { return _gfx; }
private:
  LGFX_ST7789 _gfx;
};
