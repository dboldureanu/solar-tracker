#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include "config.h"
#include "fsm.h"
#include "hardware.h"

enum class Page : uint8_t {
  Clock = 0,
  ActuatorA = 1,
  ActuatorB = 2,
  ActuatorC = 3,
  SetClock = 4,
  Config = 5,
  COUNT = 6,
};

enum class EditField : uint8_t {
  Year = 0, Month, Day, Hour, Minute, Second, COUNT
};

enum class CfgField : uint8_t {
  Month = 0, StartH, StartM, StartRaw, EndH, EndM, EndRaw, COUNT
};

class UI {
public:
  void init(hd44780_I2Cexp& lcd);

  // Call every loop iteration with encoder input.
  void handleInput(Encoder& encoder, MoveFSM& fsm, hd44780_I2Cexp& lcd);

  // Refresh display elements that update periodically.
  void refresh(MoveFSM& fsm, hd44780_I2Cexp& lcd);

  // Force-draw the current page (used at startup).
  void drawCurrentPage(hd44780_I2Cexp& lcd) { drawPage(lcd, true); }

private:
  Page    page_ = Page::Clock;
  bool    control_mode_ = false;
  uint8_t control_actuator_ = 0;

  // Clock-edit state (active only on Page::SetClock).
  bool            edit_mode_  = false;
  EditField       edit_field_ = EditField::Year;
  RTC::DateTime   edit_dt_    = {};
  int16_t         edit_accum_ = 0;

  // Config-edit state (active only on Page::Config).
  bool            cfg_edit_mode_ = false;
  uint8_t         edit_month_    = 0;        // 0..11
  MonthConfig     edit_cfg_      = {};
  CfgField        cfg_field_     = CfgField::Month;
  int16_t         cfg_accum_     = 0;

  // Accumulators for encoder sensitivity thresholds.
  int16_t nav_accum_  = 0;
  int16_t ctrl_accum_ = 0;

  // Timestamps for periodic updates.
  unsigned long last_clock_ms_ = 0;
  unsigned long last_pot_ms_   = 0;

  // --- Page rendering ---
  void drawPage(hd44780_I2Cexp& lcd, bool force = false);
  void drawClockPage(hd44780_I2Cexp& lcd, bool force = false);
  void drawActuatorPage(hd44780_I2Cexp& lcd, uint8_t index, bool entering = false,
                        MoveFSM* fsm = nullptr);
  void drawSetClockPage(hd44780_I2Cexp& lcd, bool force = false);
  void drawConfigPage(hd44780_I2Cexp& lcd, bool force = false);

  // --- Input handling ---
  void handleNavigation(int steps, MoveFSM& fsm, hd44780_I2Cexp& lcd);
  void handleControl(int steps, MoveFSM& fsm, hd44780_I2Cexp& lcd);
  void handleEdit(int steps, hd44780_I2Cexp& lcd);
  void handleCfgEdit(int steps, hd44780_I2Cexp& lcd);
  void handleButton(MoveFSM& fsm, hd44780_I2Cexp& lcd);

  // --- Helpers ---
  static uint8_t     pageToActuatorIndex(Page p);
  static uint8_t     daysInMonth(uint16_t year, uint8_t month);
  static void        adjustField(RTC::DateTime& dt, EditField f, int delta);
  static void        adjustCfgField(MonthConfig& cfg, CfgField f, int delta,
                                    uint8_t* month_index);
  static const char* monthName(uint8_t m);  // m: 0..11
  void refreshPotReading(hd44780_I2Cexp& lcd, uint8_t index);
};
