#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "config.h"

// =====================================================================
// Relay helpers
// =====================================================================

void initRelays();
void allRelaysOff();
void actuatorOff(uint8_t index);

// =====================================================================
// ADC helpers
// =====================================================================

void initAdc();
int  readAdcAvg(int pin, int samples = ADC_SAMPLES);
float adcToVolts(int raw);

// =====================================================================
// Encoder
// =====================================================================

class Encoder {
public:
  void init();
  void poll();

  // Returns accumulated direction since last call, then resets.
  // Positive = CW, negative = CCW.
  int  consumeSteps();
  bool consumeButtonPress();

private:
  uint8_t prev_ab_ = 0;
  int     steps_ = 0;
  bool    button_pressed_ = false;
  bool    button_last_ = true;  // pulled up, so idle = HIGH = true
  uint32_t last_poll_ms_ = 0;

  static int8_t decodeStep(uint8_t nibble);
  uint8_t readAB();
};

// =====================================================================
// RTC (DS3231 via I2C)
// =====================================================================

namespace RTC {
  struct DateTime {
    uint16_t year;
    uint8_t  month, day, hour, minute, second;
  };

  bool     isPresent();
  bool     read(DateTime& dt);
  bool     write(const DateTime& dt);
  void     setFromCompileTime();
}
