#include "hardware.h"

// =====================================================================
// Relay helpers
// =====================================================================

void initRelays() {
  for (int i = 0; i < RELAY_COUNT; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], HIGH);  // OFF (active LOW)
  }
}

void allRelaysOff() {
  for (int i = 0; i < RELAY_COUNT; i++) {
    digitalWrite(RELAY_PINS[i], HIGH);
  }
}

void actuatorOff(uint8_t index) {
  digitalWrite(ACTUATOR_PINS[index].dir1, HIGH);
  digitalWrite(ACTUATOR_PINS[index].dir2, HIGH);
}

// =====================================================================
// ADC helpers
// =====================================================================

void initAdc() {
  analogReadResolution(12);  // 0..4095
  for (int i = 0; i < ACTUATOR_COUNT; i++) {
    pinMode(POT_PINS[i], INPUT);
    analogSetPinAttenuation(POT_PINS[i], ADC_11db);  // ~0..3.3V
  }
}

int readAdcAvg(int pin, int samples) {
  long acc = 0;
  for (int i = 0; i < samples; i++) {
    acc += analogRead(pin);
  }
  return static_cast<int>(acc / samples);
}

float adcToVolts(int raw) {
  return raw * (3.3f / 4095.0f);
}

// =====================================================================
// Encoder
// =====================================================================

void Encoder::init() {
  pinMode(ENC_A,   INPUT_PULLUP);
  pinMode(ENC_B,   INPUT_PULLUP);
  pinMode(ENC_BTN, INPUT_PULLUP);
  prev_ab_ = readAB();
}

uint8_t Encoder::readAB() {
  uint8_t a = digitalRead(ENC_A);
  uint8_t b = digitalRead(ENC_B);
  return (a << 1) | b;
}

int8_t Encoder::decodeStep(uint8_t nibble) {
  // Quadrature gray-code lookup.
  // nibble = (prev_AB << 2) | curr_AB
  switch (nibble) {
    case 0b0001: case 0b0111: case 0b1110: case 0b1000: return +1;
    case 0b0010: case 0b1011: case 0b1101: case 0b0100: return -1;
    default: return 0;
  }
}

void Encoder::poll() {
  // Poll at most once per millisecond to avoid noise.
  uint32_t now = millis();
  if (now - last_poll_ms_ < 1) return;
  last_poll_ms_ = now;

  // Rotation
  uint8_t curr = readAB();
  uint8_t nibble = ((prev_ab_ & 0x03) << 2) | (curr & 0x03);
  steps_ += decodeStep(nibble);
  prev_ab_ = curr;

  // Button (edge detection: idle HIGH, pressed LOW)
  bool pressed = (digitalRead(ENC_BTN) == LOW);
  if (pressed && !button_last_) {
    button_pressed_ = true;
  }
  button_last_ = pressed;
}

int Encoder::consumeSteps() {
  int s = steps_;
  steps_ = 0;
  return s;
}

bool Encoder::consumeButtonPress() {
  bool p = button_pressed_;
  button_pressed_ = false;
  return p;
}

// =====================================================================
// RTC (DS3231 via I2C)
// =====================================================================

namespace RTC {

static constexpr uint8_t ADDR = 0x68;

static uint8_t dec2bcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }
static uint8_t bcd2dec(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }

bool isPresent() {
  Wire.beginTransmission(ADDR);
  return Wire.endTransmission() == 0;
}

bool read(DateTime& dt) {
  Wire.beginTransmission(ADDR);
  Wire.write(0x00);  // register: seconds
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(static_cast<int>(ADDR), 7) != 7) return false;

  uint8_t sec   = Wire.read();
  uint8_t min   = Wire.read();
  uint8_t hour  = Wire.read();
  (void)Wire.read();  // day-of-week (unused)
  uint8_t date  = Wire.read();
  uint8_t month = Wire.read();
  uint8_t year  = Wire.read();

  dt.second = bcd2dec(sec   & 0x7F);
  dt.minute = bcd2dec(min   & 0x7F);
  dt.hour   = bcd2dec(hour  & 0x3F);
  dt.day    = bcd2dec(date  & 0x3F);
  dt.month  = bcd2dec(month & 0x1F);
  dt.year   = 2000 + bcd2dec(year);
  return true;
}

bool write(const DateTime& dt) {
  Wire.beginTransmission(ADDR);
  Wire.write(0x00);  // register: seconds
  Wire.write(dec2bcd(dt.second));
  Wire.write(dec2bcd(dt.minute));
  Wire.write(dec2bcd(dt.hour & 0x3F));
  Wire.write(0x01);  // day-of-week placeholder
  Wire.write(dec2bcd(dt.day));
  Wire.write(dec2bcd(dt.month));
  Wire.write(dec2bcd(static_cast<uint8_t>(dt.year - 2000)));
  return Wire.endTransmission() == 0;
}

void setFromCompileTime() {
  const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  char mStr[4] = {__DATE__[0], __DATE__[1], __DATE__[2], '\0'};
  uint8_t month = 1 + static_cast<uint8_t>((strstr(months, mStr) - months) / 3);

  DateTime dt;
  dt.day    = static_cast<uint8_t>(atoi(__DATE__ + 4));
  dt.year   = static_cast<uint16_t>(atoi(__DATE__ + 7));
  dt.hour   = static_cast<uint8_t>(atoi(__TIME__ + 0));
  dt.minute = static_cast<uint8_t>(atoi(__TIME__ + 3));
  dt.second = static_cast<uint8_t>(atoi(__TIME__ + 6));
  dt.month  = month;
  write(dt);
}

}  // namespace RTC
