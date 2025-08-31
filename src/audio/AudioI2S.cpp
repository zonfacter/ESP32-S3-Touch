// ============================================================================
// File: src/audio/AudioI2S.cpp
// ----------------------------------------------------------------------------
#include "AudioI2S.h"
#include <driver/i2s.h>
#include <math.h>

static i2s_port_t I2S_PORT = I2S_NUM_0;
static uint32_t SR = 22050;

bool AudioI2S::begin(uint32_t rate){
  SR = rate;
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = (int)SR,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };
  i2s_pin_config_t pins = {
    .mck_io_num = I2S_PIN_NO_CHANGE,
    .bck_io_num = PIN_I2S_BCK,
    .ws_io_num  = PIN_I2S_LRCK,
    .data_out_num = PIN_I2S_DIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  if (i2s_driver_install(I2S_PORT, &cfg, 0, nullptr) != ESP_OK) return false;
  if (i2s_set_pin(I2S_PORT, &pins) != ESP_OK) return false;
  i2s_zero_dma_buffer(I2S_PORT);
  return true;
}

void AudioI2S::toneHz(float hz, uint16_t ms, float amp){
  if (hz <= 0) { delay(ms); return; }
  const int N = (int)((SR * ms) / 1000);
  const float twoPiOverSR = 2.0f * (float)M_PI / (float)SR;
  const int BUF = 256;
  int16_t buf[BUF];
  float phase = 0.f;

  // kurze, nicht-blockierende Writes (Timeout 10ms). Droppt zur Not Samples.
  for (int i=0; i<N; ){
    int chunk = min(BUF, N - i);
    for (int j=0; j<chunk; j++, i++){
      float s = sinf(phase) * amp;
      buf[j] = (int16_t)(s * 32767);
      phase += twoPiOverSR * hz;
      if (phase > 2*M_PI) phase -= 2*M_PI;
    }
    size_t written = 0;
    // 10 ms Timeout statt portMAX_DELAY
    i2s_write(I2S_PORT, (const char*)buf, chunk*sizeof(int16_t),
              &written, 10 / portTICK_PERIOD_MS);
    // wenn Puffer voll: written kann < chunk sein -> rest wird im nächsten Zyklus erzeugt
  }
}

void AudioI2S::playGesture(const GestureType g){
  unsigned long now = millis();
  if (now < _gateUntil) return;     // simple flood-guard

  switch (g){
    case GestureType::Tap:          toneHz(1200, 60); break;
    case GestureType::DoubleTap:    toneHz(1200, 60); toneHz(0, 40); toneHz(1200, 60); break;
    case GestureType::LongPress:    toneHz(600, 200); break;
    case GestureType::SwipeLeft:
    case GestureType::SwipeRight:   toneHz(900, 80); break;
    case GestureType::SwipeUp:
    case GestureType::SwipeDown:    toneHz(700, 80); break;
    case GestureType::PinchIn:      toneHz(500, 80); toneHz(0, 40); toneHz(400, 100); break;
    case GestureType::PinchOut:     toneHz(400, 80); toneHz(0, 40); toneHz(500, 100); break;
    case GestureType::RotateCW:     toneHz(1000, 70); toneHz(0, 30); toneHz(1200, 70); break;
    case GestureType::RotateCCW:    toneHz(1200, 70); toneHz(0, 30); toneHz(1000, 70); break;
    case GestureType::TwoFingerTap: toneHz(1000, 60); toneHz(0, 20); toneHz(1000, 60); break;
    case GestureType::ThreeFingerTap:toneHz(800, 120); break;
    default: break;
  }

  // nächstes Signal frühestens in ~150ms (verhindert Ton-Staus im Loop)
  _gateUntil = now + 150;
}