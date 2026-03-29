#pragma once

// ===== Relay Pins (active LOW) =====
// Each actuator has two direction relays.
struct ActuatorPins {
  int dir1;
  int dir2;
};

constexpr ActuatorPins ACTUATOR_PINS[] = {
  {13, 14},  // Actuator A
  {16, 17},  // Actuator B
  {18, 19},  // Actuator C
};
constexpr int ACTUATOR_COUNT = 3;

// All relay GPIO pins (actuators + buck), for bulk init.
constexpr int RELAY_PINS[] = {13, 14, 16, 17, 18, 19, 23, 25};
constexpr int RELAY_COUNT  = sizeof(RELAY_PINS) / sizeof(RELAY_PINS[0]);

// Buck converter control relays
constexpr int BUCK_MAIN_PIN = 23;
constexpr int BUCK_PRE_PIN  = 25;

// ===== Encoder Pins =====
constexpr int ENC_A   = 32;
constexpr int ENC_B   = 33;
constexpr int ENC_BTN = 26;  // LOW when pressed (INPUT_PULLUP)

// ===== Potentiometer ADC Pins (ESP32 ADC1) =====
constexpr int POT_PINS[] = {34, 35, 36};  // A, B, C

// ===== I2C =====
constexpr int I2C_SDA = 21;
constexpr int I2C_SCL = 22;

// ===== INA226 Alert (optional, not used yet) =====
constexpr int INA_ALERT_PIN = 27;

// ===== LCD =====
constexpr int LCD_COLS = 20;
constexpr int LCD_ROWS = 4;

// ===== Encoder Sensitivity =====
// Navigation mode: ticks required to change page
constexpr int NAV_TICKS_PER_ACTION = 4;
// Control mode: ticks required to trigger actuator
constexpr int CTRL_TICKS_PER_PULSE = 2;

// ===== Actuator Timing =====
// Initial movement duration when first triggered
constexpr unsigned long PULSE_MS = 30000UL;          // 30 seconds
// Time added/removed per encoder twist while running
constexpr unsigned long MOVE_INCREMENT_MS = 30000UL;  // +/- 30 seconds
// Maximum total run time
constexpr unsigned long MAX_MOVE_MS = 60000UL;        // 60 seconds

// ===== Buck Power Sequencing Timing =====
constexpr unsigned long PRECHARGE_MS      = 400;   // PRE relay -> MAIN relay
constexpr unsigned long MAIN_STAB_MS      = 150;   // MAIN relay -> actuator ON
constexpr unsigned long POST_ACT_PAUSE_MS = 100;   // actuator OFF -> MAIN relay OFF
constexpr unsigned long BUCK_SPINDOWN_MS  = 150;   // MAIN OFF -> PRE OFF

// ===== Periodic Update Intervals =====
constexpr unsigned long CLOCK_REFRESH_MS = 1000;   // clock page refresh
constexpr unsigned long POT_REFRESH_MS   = 200;    // potentiometer readout refresh

// ===== ADC =====
constexpr int ADC_SAMPLES = 12;  // samples to average per reading
