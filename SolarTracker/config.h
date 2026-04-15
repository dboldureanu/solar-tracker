#pragma once

// ===== Relay Pins (active LOW) =====
// Each actuator has two direction relays, named by the tracking direction
// they drive the panels toward: east = morning/low-pot, west = evening/high-pot.
// Actuator C is wired with swapped polarity relative to A/B; the swap is
// expressed here so the rest of the code can stay direction-semantic.
struct ActuatorPins {
  int east;
  int west;
};

constexpr ActuatorPins ACTUATOR_PINS[] = {
  {13, 14},  // Actuator A
  {16, 17},  // Actuator B
  {19, 18},  // Actuator C (swapped in software to match wiring)
};
constexpr int ACTUATOR_COUNT = 3;

// Potentiometer calibration: raw ADC values at physical end stops.
// Measured 2026-04-13 with panels driven manually to each extreme.
struct PotCalibration {
  int east_raw;  // extreme east (morning)
  int west_raw;  // extreme west (evening)
};

constexpr PotCalibration POT_CAL[] = {
  {430, 3140},  // A
  {429, 3200},  // B
  {400, 3190},  // C
};

// ===== Monthly tracking window =====
// For each month: the tracking time window and the pot setpoints at its
// endpoints. Between start and end, the target pot is linearly interpolated.
// Outside the window, auto-tracking is idle.
struct MonthConfig {
  uint8_t  start_h;
  uint8_t  start_m;
  uint16_t start_raw;   // target pot at start-of-window (east-ish)
  uint8_t  end_h;
  uint8_t  end_m;
  uint16_t end_raw;     // target pot at end-of-window (west-ish)
};

constexpr uint8_t MONTH_COUNT = 12;

// Encoder pulse -> raw ADC delta when editing a pot setpoint.
constexpr int POT_EDIT_STEP = 10;

// ===== System (global auto-tracking) config =====
// Stored separately from per-month config in NVS.
struct SystemConfig {
  bool    auto_enabled;   // master on/off for auto-tracking
  uint8_t tick_min;       // evaluation interval, minutes (1..180)
};

// Tick interval bounds.
constexpr uint8_t TICK_MIN_MIN = 1;
constexpr uint8_t TICK_MIN_MAX = 180;

// ===== Auto-tracking tuning =====
// Dead-band around the target pot reading; |target - actual| <= this means
// "aligned, don't move". Units: raw 12-bit ADC counts.
constexpr int AUTO_DEADBAND_RAW = 20;

// Stall detection: if the pot hasn't moved by this many raw counts within
// AUTO_STALL_WINDOW_MS, abort the current actuator's move.
constexpr int           AUTO_STALL_DELTA_RAW = 10;
constexpr unsigned long AUTO_STALL_WINDOW_MS = 5000;

// Max consecutive FSM pulses per actuator per auto-track pass. Each pulse
// is PULSE_MS long (30s), so 3 pulses covers a full stroke (~90s).
constexpr uint8_t AUTO_MAX_PULSES = 3;

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
// 16 samples * ~1250us per sample ~= 20 ms, which covers one full 50 Hz
// mains cycle. This averages out hum and PWM aliasing that back-to-back
// samples would miss.
constexpr int           ADC_SAMPLES              = 16;
constexpr unsigned long ADC_INTERSAMPLE_US       = 1250;

// Median-of-N reads on top of readAdcAvg, for the few decision points
// where the occasional spike (e.g. relay click) would cause a bad move.
constexpr int ADC_STABLE_SAMPLES = 5;  // must be odd
