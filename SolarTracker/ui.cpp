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
    if (control_mode_) {
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
      fsm.start(control_actuator_, ACTUATOR_PINS[control_actuator_].dir2, lcd);  // CW -> dir2
      drawActuatorPage(lcd, control_actuator_, false, &fsm);
    } else if (ctrl_accum_ <= -CTRL_TICKS_PER_PULSE) {
      ctrl_accum_ = 0;
      fsm.start(control_actuator_, ACTUATOR_PINS[control_actuator_].dir1, lcd);  // CCW -> dir1
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

  // Actuator pages (not in control mode): refresh potentiometer.
  if (!control_mode_ && page_ != Page::Clock) {
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
    default: break;
  }
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
