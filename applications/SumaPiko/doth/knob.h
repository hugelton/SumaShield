#include <stdint.h>
#include <stdlib.h>
#include <Arduino.h>

class Knob {
  uint8_t input;
  uint16_t val[2];  // 0 = current value, 1 = last value
  uint16_t val_max;
  uint16_t alpha;
  uint16_t startup;
  bool changed;

 public:
  void Init(uint8_t input_, uint16_t alpha_) {
    input = input_;  // 0 or 1 (for ADC 26 or 27)
    alpha = alpha_;
    // Arduino IDE: no need for adc_gpio_init, just use analogRead
    val_max = 4095;
    startup = 800;
  }

  void Reset() { startup = 800; }

  uint16_t Value() { return val[1]; }
  uint16_t ValueMax() { return val_max; }

  void Read() {
    // Arduino IDE: use analogRead instead of adc_read
    uint16_t adc = analogRead(input + 26);  // ADC 26 or 27
    val[0] = adc;
    changed = abs((int)val[0] - (int)val[1]) > 100;
    if (changed) {
      val[1] = val[0];
    }
  }

  bool Changed() {
    // prevent reading on startup
    if (startup > 0) {
      startup--;
      return false;
    }
    return changed;
  }
};
