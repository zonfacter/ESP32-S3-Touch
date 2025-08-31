// ============================================================================
// File: src/config/pins.h
// Project: Waveshare ESP32-S3 Touch LCD 2.8 – Modularbasis
// Author: ChatGPT (Thomas' Assistent)
// License: MIT
// ----------------------------------------------------------------------------
// Purpose: Zentrale Pin-/Adresskonfiguration für Display, Touch, IMU und Audio
// Hardware: ST7789T3 LCD, CST328 Touch, QMI8658C IMU, APA2026 KD2412X Amp
// Note: Werte basieren auf Thomas' funktionierenden Konfigurationen
// ============================================================================
#pragma once
#include <Arduino.h>

// ---------------------------- Display (ST7789T3) ----------------------------
// SPI3 (FSPI) Pins laut funktionierender Konfiguration
static constexpr int PIN_LCD_SCLK = 40;
static constexpr int PIN_LCD_MOSI = 45;
static constexpr int PIN_LCD_MISO = -1; // nicht genutzt
static constexpr int PIN_LCD_DC   = 41;
static constexpr int PIN_LCD_CS   = 42;
static constexpr int PIN_LCD_RST  = 39;

// ---------------------------- Touch (CST328, I2C1) -------------------------
static constexpr int PIN_TOUCH_SDA = 1;
static constexpr int PIN_TOUCH_SCL = 3;
static constexpr int PIN_TOUCH_INT = 4;
static constexpr int PIN_TOUCH_RST = 2;

// ---------------------------- IMU (QMI8658C, I2C0) -------------------------
static constexpr int PIN_IMU_SCL = 10;
static constexpr int PIN_IMU_SDA = 11;
static constexpr int PIN_IMU_INT2 = 12;
static constexpr int PIN_IMU_INT1 = 13;

// ---------------------------- Audio (I2S, APA2026) -------------------------
// LRCK(WS) = IO38, DIN = IO47, BCK = IO48
static constexpr int PIN_I2S_LRCK = 38;
static constexpr int PIN_I2S_DIN  = 47;
static constexpr int PIN_I2S_BCK  = 48;

// ---------------------------- RS485 -------------------------
static constexpr int PIN_RS485_TX = 15;  // UART1 TX
static constexpr int PIN_RS485_RX = 14;  // UART1 RX
static constexpr int PIN_RS485_DE = 16;  // DE/RE (HIGH=TX)
static constexpr int RS485_UART_PORT = 1;

// ---------------------------- I2C Adressen (Waveshare FAQ) -----------------
// Onboard I2C devices are expected at: 0x51, 0x6B, 0x7E
static constexpr uint8_t I2C_ADDR_EXPECTED[3] = { 0x51, 0x6B, 0x7E };

// Touch Controller Adresse (CST328)
static constexpr uint8_t CST328_I2C_ADDR = 0x1A; // bestätigt

