#include "ui.h"

void UI::init(hd44780_I2Cexp& lcd) {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Solar Tracker UI");
  lcd.setCursor(0, 1); lcd.print("Pages + Control");
  lcd.setCursor(0, 2); lcd.print("Encoder: nav/ctrl");
  lcd.setCursor(0, 3); lcd.print("Init...");

  page_ = Page::Clock;
  control_mode_ = false;
}

// =====================================================================
// Input handling
// =====================================================================

void UI::handleInput(Encoder& encoder, MoveFSM& fsm, hd44780_I2Cexp& lcd) {
  encoder.poll();

  int steps = encoder.consumeSteps();
  if (steps != 0) {
    if (edit_mode_) {
      handleEdit(steps, lcd);
    } else if (cfg_edit_mode_) {
      handleCfgEdit(steps, lcd);
    } else if (control_mode_) {
      handleControl(steps, fsm, lcd);
    } else {
      handleNavigation(steps, fsm, lcd);
    }
  }

  if (encoder.consumeButtonPress()) {
    handleButton(fsm, lcd);
  }
}

void UI::handleNavigation(int steps, MoveFSM& fsm, hd44780_I2Cexp& lcd) {
  nav_accum_ += steps;

  if (nav_accum_ >= NAV_TICKS_PER_ACTION) {
    nav_accum_ = 0;
    page_ = static_cast<Page>((static_cast<int>(page_) + 1) % static_cast<int>(Page::COUNT));
    drawPage(lcd, true);
  } else if (nav_accum_ <= -NAV_TICKS_PER_ACTION) {
    nav_accum_ = 0;
    int p = static_cast<int>(page_) - 1;
    if (p < 0) p = static_cast<int>(Page::COUNT) - 1;
    page_ = static_cast<Page>(p);
    drawPage(lcd, true);
  }
}

void UI::handleControl(int steps, MoveFSM& fsm, hd44780_I2Cexp& lcd) {
  ctrl_accum_ += steps;

  if (!fsm.isActive()) {
    // Not moving yet — start a new move when threshold is reached.
    if (ctrl_accum_ >= CTRL_TICKS_PER_PULSE) {
      ctrl_accum_ = 0;
      fsm.start(control_actuator_, ACTUATOR_PINS[control_actuator_].west, lcd);  // CW -> west
      drawActuatorPage(lcd, control_actuator_, false, &fsm);
    } else if (ctrl_accum_ <= -CTRL_TICKS_PER_PULSE) {
      ctrl_accum_ = 0;
      fsm.start(control_actuator_, ACTUATOR_PINS[control_actuator_].east, lcd);  // CCW -> east
      drawActuatorPage(lcd, control_actuator_, false, &fsm);
    }
  } else if (fsm.phase() == MovePhase::ActOn) {
    // Already running — CW adds time, CCW subtracts.
    if (ctrl_accum_ >= CTRL_TICKS_PER_PULSE) {
      ctrl_accum_ = 0;
      fsm.extendTime(lcd);
    } else if (ctrl_accum_ <= -CTRL_TICKS_PER_PULSE) {
      ctrl_accum_ = 0;
      fsm.reduceTime(lcd);
    }
  }
  // Rotations during buck precharge/stabilization are ignored (short phases).
}

void UI::handleButton(MoveFSM& fsm, hd44780_I2Cexp& lcd) {
  if (page_ == Page::Clock) return;  // no action on clock page

  if (page_ == Page::Config) {
    if (!cfg_edit_mode_) {
      // Seed working copy from current month (from RTC if we can read it).
      uint8_t m = 0;
      RTC::DateTime now;
      if (RTC::read(now) && now.month >= 1 && now.month <= 12) {
        m = now.month - 1;
      }
      edit_month_    = m;
      edit_cfg_      = Settings::month(edit_month_);
      cfg_field_     = CfgField::Month;
      cfg_accum_     = 0;
      cfg_edit_mode_ = true;
      drawConfigPage(lcd, true);
      return;
    }

    // Already editing: advance field, or save and exit after the last one.
    uint8_t next = static_cast<uint8_t>(cfg_field_) + 1;
    if (next >= static_cast<uint8_t>(CfgField::COUNT)) {
      Settings::month(edit_month_) = edit_cfg_;
      Settings::save();
      cfg_edit_mode_ = false;
      drawConfigPage(lcd, true);
    } else {
      cfg_field_ = static_cast<CfgField>(next);
      cfg_accum_ = 0;
      drawConfigPage(lcd, true);
    }
    return;
  }

  if (page_ == Page::SetClock) {
    if (!edit_mode_) {
      // Enter edit mode: seed working copy from RTC (or leave zeros if read fails).
      RTC::DateTime dt;
      if (RTC::read(dt)) {
        edit_dt_ = dt;
      } else {
        edit_dt_ = {2026, 1, 1, 0, 0, 0};
      }
      edit_field_ = EditField::Year;
      edit_accum_ = 0;
      edit_mode_  = true;
      drawSetClockPage(lcd, true);
      return;
    }

    // Already editing: advance to next field, or commit after the last one.
    uint8_t next = static_cast<uint8_t>(edit_field_) + 1;
    if (next >= static_cast<uint8_t>(EditField::COUNT)) {
      RTC::write(edit_dt_);
      edit_mode_ = false;
      drawSetClockPage(lcd, true);
    } else {
      edit_field_ = static_cast<EditField>(next);
      edit_accum_ = 0;
      drawSetClockPage(lcd, true);
    }
    return;
  }

  if (!control_mode_) {
    control_mode_ = true;
    ctrl_accum_ = 0;
    control_actuator_ = pageToActuatorIndex(page_);
    actuatorOff(control_actuator_);
    drawActuatorPage(lcd, control_actuator_, true);
  } else {
    control_mode_ = false;
    ctrl_accum_ = 0;
    actuatorOff(control_actuator_);
    drawPage(lcd, true);
  }
}

// =====================================================================
// Display rendering
// =====================================================================

void UI::refresh(MoveFSM& fsm, hd44780_I2Cexp& lcd) {
  // Clock page: refresh once per second.
  if (page_ == Page::Clock) {
    drawClockPage(lcd);
  }

  // SetClock page, view mode: refresh once per second (same cadence as Clock).
  if (page_ == Page::SetClock && !edit_mode_) {
    drawSetClockPage(lcd);
  }

  // Config page, view mode: refresh once per second so the clock-driven
  // "current month" label stays live.
  if (page_ == Page::Config && !cfg_edit_mode_) {
    drawConfigPage(lcd);
  }

  // Actuator pages (not in control mode): refresh potentiometer.
  if (!control_mode_ && page_ != Page::Clock && page_ != Page::SetClock
      && page_ != Page::Config) {
    unsigned long now = millis();
    if (now - last_pot_ms_ > POT_REFRESH_MS) {
      last_pot_ms_ = now;
      refreshPotReading(lcd, pageToActuatorIndex(page_));
    }
  }
}

void UI::drawPage(hd44780_I2Cexp& lcd, bool force) {
  switch (page_) {
    case Page::Clock:      drawClockPage(lcd, force); break;
    case Page::ActuatorA:  drawActuatorPage(lcd, 0, force); break;
    case Page::ActuatorB:  drawActuatorPage(lcd, 1, force); break;
    case Page::ActuatorC:  drawActuatorPage(lcd, 2, force); break;
    case Page::SetClock:   drawSetClockPage(lcd, force); break;
    case Page::Config:     drawConfigPage(lcd, force); break;
    default: break;
  }
}

void UI::handleEdit(int steps, hd44780_I2Cexp& lcd) {
  edit_accum_ += steps;

  // 2 encoder ticks per unit change (matches CTRL sensitivity).
  while (edit_accum_ >= CTRL_TICKS_PER_PULSE) {
    edit_accum_ -= CTRL_TICKS_PER_PULSE;
    adjustField(edit_dt_, edit_field_, +1);
  }
  while (edit_accum_ <= -CTRL_TICKS_PER_PULSE) {
    edit_accum_ += CTRL_TICKS_PER_PULSE;
    adjustField(edit_dt_, edit_field_, -1);
  }

  drawSetClockPage(lcd, true);
}

void UI::drawSetClockPage(hd44780_I2Cexp& lcd, bool force) {
  // View mode: mirror the Clock page layout but offer an Edit action.
  if (!edit_mode_) {
    unsigned long now = millis();
    if (!force && (now - last_clock_ms_) < CLOCK_REFRESH_MS) return;
    last_clock_ms_ = now;

    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Set Date & Time");

    RTC::DateTime dt;
    bool ok = RTC::isPresent() && RTC::read(dt);
    if (!ok) {
      lcd.setCursor(0, 1); lcd.print("RTC unavailable");
    } else {
      lcd.setCursor(0, 1);
      lcd.print(dt.year); lcd.print('-');
      if (dt.month < 10) lcd.print('0'); lcd.print(dt.month); lcd.print('-');
      if (dt.day   < 10) lcd.print('0'); lcd.print(dt.day);

      lcd.setCursor(0, 2);
      if (dt.hour   < 10) lcd.print('0'); lcd.print(dt.hour);   lcd.print(':');
      if (dt.minute < 10) lcd.print('0'); lcd.print(dt.minute); lcd.print(':');
      if (dt.second < 10) lcd.print('0'); lcd.print(dt.second);
    }

    lcd.setCursor(0, 3); lcd.print("Btn: Edit           ");
    return;
  }

  // Edit mode: show the working copy with brackets around the active field.
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Edit Date & Time");

  const auto& dt = edit_dt_;
  char buf[21];

  // Line 1: Date with optional brackets on Y / M / D.
  switch (edit_field_) {
    case EditField::Year:
      snprintf(buf, sizeof(buf), "Date:[%04u]-%02u-%02u",
               dt.year, dt.month, dt.day);
      break;
    case EditField::Month:
      snprintf(buf, sizeof(buf), "Date: %04u-[%02u]-%02u",
               dt.year, dt.month, dt.day);
      break;
    case EditField::Day:
      snprintf(buf, sizeof(buf), "Date: %04u-%02u-[%02u]",
               dt.year, dt.month, dt.day);
      break;
    default:
      snprintf(buf, sizeof(buf), "Date: %04u-%02u-%02u  ",
               dt.year, dt.month, dt.day);
      break;
  }
  lcd.setCursor(0, 1); lcd.print(buf);

  // Line 2: Time with optional brackets on H / M / S.
  switch (edit_field_) {
    case EditField::Hour:
      snprintf(buf, sizeof(buf), "Time:[%02u]:%02u:%02u",
               dt.hour, dt.minute, dt.second);
      break;
    case EditField::Minute:
      snprintf(buf, sizeof(buf), "Time: %02u:[%02u]:%02u",
               dt.hour, dt.minute, dt.second);
      break;
    case EditField::Second:
      snprintf(buf, sizeof(buf), "Time: %02u:%02u:[%02u]",
               dt.hour, dt.minute, dt.second);
      break;
    default:
      snprintf(buf, sizeof(buf), "Time: %02u:%02u:%02u  ",
               dt.hour, dt.minute, dt.second);
      break;
  }
  lcd.setCursor(0, 2); lcd.print(buf);

  lcd.setCursor(0, 3);
  if (edit_field_ == EditField::Second) {
    lcd.print("Rot:chg Btn:save   ");
  } else {
    lcd.print("Rot:chg Btn:next   ");
  }
}

uint8_t UI::daysInMonth(uint16_t year, uint8_t month) {
  static const uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (month < 1 || month > 12) return 31;
  if (month == 2) {
    bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    return leap ? 29 : 28;
  }
  return days[month - 1];
}

void UI::adjustField(RTC::DateTime& dt, EditField f, int delta) {
  auto wrap = [](int v, int lo, int hi) {
    int span = hi - lo + 1;
    v -= lo;
    v = ((v % span) + span) % span;
    return v + lo;
  };
  auto clamp = [](int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
  };

  switch (f) {
    case EditField::Year:
      dt.year = static_cast<uint16_t>(clamp(dt.year + delta, 2020, 2099));
      break;
    case EditField::Month:
      dt.month = static_cast<uint8_t>(wrap(dt.month + delta, 1, 12));
      break;
    case EditField::Day:
      dt.day = static_cast<uint8_t>(wrap(dt.day + delta, 1, daysInMonth(dt.year, dt.month)));
      break;
    case EditField::Hour:
      dt.hour = static_cast<uint8_t>(wrap(dt.hour + delta, 0, 23));
      break;
    case EditField::Minute:
      dt.minute = static_cast<uint8_t>(wrap(dt.minute + delta, 0, 59));
      break;
    case EditField::Second:
      dt.second = static_cast<uint8_t>(wrap(dt.second + delta, 0, 59));
      break;
    default: break;
  }

  // If year/month changed under a too-large day, clamp it.
  uint8_t dim = daysInMonth(dt.year, dt.month);
  if (dt.day > dim) dt.day = dim;
}

void UI::drawClockPage(hd44780_I2Cexp& lcd, bool force) {
  unsigned long now = millis();
  if (!force && (now - last_clock_ms_) < CLOCK_REFRESH_MS) return;
  last_clock_ms_ = now;

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Date & Time");

  if (!RTC::isPresent()) {
    lcd.setCursor(0, 2); lcd.print("RTC not found");
    return;
  }

  RTC::DateTime dt;
  if (!RTC::read(dt)) {
    lcd.setCursor(0, 2); lcd.print("RTC read error");
    return;
  }

  // Line 1: YYYY-MM-DD
  lcd.setCursor(0, 1);
  lcd.print(dt.year); lcd.print('-');
  if (dt.month < 10) lcd.print('0'); lcd.print(dt.month); lcd.print('-');
  if (dt.day < 10)   lcd.print('0'); lcd.print(dt.day);

  // Line 2: HH:MM:SS
  lcd.setCursor(0, 2);
  if (dt.hour < 10)   lcd.print('0'); lcd.print(dt.hour);   lcd.print(':');
  if (dt.minute < 10) lcd.print('0'); lcd.print(dt.minute); lcd.print(':');
  if (dt.second < 10) lcd.print('0'); lcd.print(dt.second);

  lcd.setCursor(0, 3);
  lcd.print("Rotate: pages | Btn:-");
}

void UI::drawActuatorPage(hd44780_I2Cexp& lcd, uint8_t index, bool entering,
                          MoveFSM* fsm) {
  if (entering) lcd.clear();

  // Line 0: Actuator name
  lcd.setCursor(0, 0);
  lcd.print("Actuator ");
  lcd.print(static_cast<char>('A' + index));

  // Line 1: Potentiometer reading
  refreshPotReading(lcd, index);

  // Lines 2-3: Mode-dependent
  lcd.setCursor(0, 2);
  if (!control_mode_) {
    lcd.print("Rotate: next/prev  ");
    lcd.setCursor(0, 3);
    lcd.print("Btn: Control mode  ");
  } else {
    lcd.print("CONTROL: rotate => ");
    lcd.setCursor(0, 3);
    if (fsm && fsm->isActive()) {
      // Remaining time shown by FSM tick; don't overwrite here.
    } else {
      lcd.print("Dir: CCW<-  ->CW   ");
    }
  }
}

void UI::refreshPotReading(hd44780_I2Cexp& lcd, uint8_t index) {
  int raw = readAdcAvg(POT_PINS[index]);
  float v = adcToVolts(raw);

  lcd.setCursor(0, 1);
  lcd.print("POT: ");
  if (raw < 1000) lcd.print(' ');
  if (raw < 100)  lcd.print(' ');
  if (raw < 10)   lcd.print(' ');
  lcd.print(raw);
  lcd.print("  ");
  lcd.print(v, 2);
  lcd.print("V   ");
}

uint8_t UI::pageToActuatorIndex(Page p) {
  switch (p) {
    case Page::ActuatorA: return 0;
    case Page::ActuatorB: return 1;
    case Page::ActuatorC: return 2;
    default: return 0;
  }
}

const char* UI::monthName(uint8_t m) {
  static const char* const names[12] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
  };
  if (m >= 12) return "???";
  return names[m];
}

void UI::handleCfgEdit(int steps, hd44780_I2Cexp& lcd) {
  cfg_accum_ += steps;

  while (cfg_accum_ >= CTRL_TICKS_PER_PULSE) {
    cfg_accum_ -= CTRL_TICKS_PER_PULSE;
    adjustCfgField(edit_cfg_, cfg_field_, +1, &edit_month_);
    if (cfg_field_ == CfgField::Month) {
      edit_cfg_ = Settings::month(edit_month_);
    }
  }
  while (cfg_accum_ <= -CTRL_TICKS_PER_PULSE) {
    cfg_accum_ += CTRL_TICKS_PER_PULSE;
    adjustCfgField(edit_cfg_, cfg_field_, -1, &edit_month_);
    if (cfg_field_ == CfgField::Month) {
      edit_cfg_ = Settings::month(edit_month_);
    }
  }

  drawConfigPage(lcd, true);
}

void UI::adjustCfgField(MonthConfig& cfg, CfgField f, int delta,
                        uint8_t* month_index) {
  auto wrap = [](int v, int lo, int hi) {
    int span = hi - lo + 1;
    v -= lo;
    v = ((v % span) + span) % span;
    return v + lo;
  };
  auto clamp = [](int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
  };

  switch (f) {
    case CfgField::Month:
      *month_index = static_cast<uint8_t>(wrap(*month_index + delta, 0, 11));
      break;
    case CfgField::StartH:
      cfg.start_h = static_cast<uint8_t>(wrap(cfg.start_h + delta, 0, 23));
      break;
    case CfgField::StartM:
      cfg.start_m = static_cast<uint8_t>(wrap(cfg.start_m + delta, 0, 59));
      break;
    case CfgField::StartRaw:
      cfg.start_raw = static_cast<uint16_t>(
          clamp(static_cast<int>(cfg.start_raw) + delta * POT_EDIT_STEP, 0, 4095));
      break;
    case CfgField::EndH:
      cfg.end_h = static_cast<uint8_t>(wrap(cfg.end_h + delta, 0, 23));
      break;
    case CfgField::EndM:
      cfg.end_m = static_cast<uint8_t>(wrap(cfg.end_m + delta, 0, 59));
      break;
    case CfgField::EndRaw:
      cfg.end_raw = static_cast<uint16_t>(
          clamp(static_cast<int>(cfg.end_raw) + delta * POT_EDIT_STEP, 0, 4095));
      break;
    default: break;
  }
}

void UI::drawConfigPage(hd44780_I2Cexp& lcd, bool force) {
  if (!cfg_edit_mode_) {
    // View mode: show the month matching the RTC, refreshed once per second.
    unsigned long now = millis();
    if (!force && (now - last_clock_ms_) < CLOCK_REFRESH_MS) return;
    last_clock_ms_ = now;

    uint8_t m = 0;
    RTC::DateTime dt;
    if (RTC::read(dt) && dt.month >= 1 && dt.month <= 12) m = dt.month - 1;
    const MonthConfig& c = Settings::month(m);

    char buf[21];
    lcd.clear();

    snprintf(buf, sizeof(buf), "Config: %s", monthName(m));
    lcd.setCursor(0, 0); lcd.print(buf);

    snprintf(buf, sizeof(buf), "S %02u:%02u %4u %.2fV",
             c.start_h, c.start_m, c.start_raw, adcToVolts(c.start_raw));
    lcd.setCursor(0, 1); lcd.print(buf);

    snprintf(buf, sizeof(buf), "E %02u:%02u %4u %.2fV",
             c.end_h, c.end_m, c.end_raw, adcToVolts(c.end_raw));
    lcd.setCursor(0, 2); lcd.print(buf);

    lcd.setCursor(0, 3); lcd.print("Btn: Edit           ");
    return;
  }

  // Edit mode: render working copy with brackets around the active field.
  lcd.clear();
  char buf[21];

  // Line 0: month, bracketed when Month field is active.
  if (cfg_field_ == CfgField::Month) {
    snprintf(buf, sizeof(buf), "Cfg: [%s]", monthName(edit_month_));
  } else {
    snprintf(buf, sizeof(buf), "Cfg:  %s",  monthName(edit_month_));
  }
  lcd.setCursor(0, 0); lcd.print(buf);

  // Line 1: Start row.
  const MonthConfig& c = edit_cfg_;
  switch (cfg_field_) {
    case CfgField::StartH:
      snprintf(buf, sizeof(buf), "S [%02u]:%02u %4u",
               c.start_h, c.start_m, c.start_raw);
      break;
    case CfgField::StartM:
      snprintf(buf, sizeof(buf), "S %02u:[%02u] %4u",
               c.start_h, c.start_m, c.start_raw);
      break;
    case CfgField::StartRaw:
      snprintf(buf, sizeof(buf), "S %02u:%02u [%4u]",
               c.start_h, c.start_m, c.start_raw);
      break;
    default:
      snprintf(buf, sizeof(buf), "S %02u:%02u  %4u ",
               c.start_h, c.start_m, c.start_raw);
      break;
  }
  lcd.setCursor(0, 1); lcd.print(buf);

  // Line 2: End row.
  switch (cfg_field_) {
    case CfgField::EndH:
      snprintf(buf, sizeof(buf), "E [%02u]:%02u %4u",
               c.end_h, c.end_m, c.end_raw);
      break;
    case CfgField::EndM:
      snprintf(buf, sizeof(buf), "E %02u:[%02u] %4u",
               c.end_h, c.end_m, c.end_raw);
      break;
    case CfgField::EndRaw:
      snprintf(buf, sizeof(buf), "E %02u:%02u [%4u]",
               c.end_h, c.end_m, c.end_raw);
      break;
    default:
      snprintf(buf, sizeof(buf), "E %02u:%02u  %4u ",
               c.end_h, c.end_m, c.end_raw);
      break;
  }
  lcd.setCursor(0, 2); lcd.print(buf);

  // Line 3: context-sensitive hint, with live voltage for the active pot.
  const bool last_field = (static_cast<uint8_t>(cfg_field_) + 1
                           == static_cast<uint8_t>(CfgField::COUNT));
  const char* btn_lbl = last_field ? "Btn:save" : "Btn:next";
  lcd.setCursor(0, 3);
  if (cfg_field_ == CfgField::StartRaw) {
    snprintf(buf, sizeof(buf), "%.2fV  %s   ",
             adcToVolts(c.start_raw), btn_lbl);
    lcd.print(buf);
  } else if (cfg_field_ == CfgField::EndRaw) {
    snprintf(buf, sizeof(buf), "%.2fV  %s   ",
             adcToVolts(c.end_raw), btn_lbl);
    lcd.print(buf);
  } else {
    snprintf(buf, sizeof(buf), "Rot:chg  %s   ", btn_lbl);
    lcd.print(buf);
  }
}
