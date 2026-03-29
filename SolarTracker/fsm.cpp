#include "fsm.h"
#include "hardware.h"
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

void MoveFSM::start(uint8_t actuatorIndex, int directionPin, hd44780_I2Cexp& lcd) {
  if (active_) return;

  active_   = true;
  phase_    = MovePhase::PreOn;
  actuator_ = actuatorIndex;
  act_pin_  = directionPin;

  // Safety: ensure actuator relays are off before powering the buck.
  actuatorOff(actuatorIndex);

  // Step 1: Buck PRE relay ON.
  digitalWrite(BUCK_PRE_PIN, LOW);
  next_at_ = millis() + PRECHARGE_MS;

  lcd.setCursor(0, 3); lcd.print("Powering buck...   ");
}

void MoveFSM::stop(hd44780_I2Cexp& lcd) {
  if (phase_ != MovePhase::ActOn) return;

  digitalWrite(act_pin_, HIGH);  // actuator OFF
  advanceTo(MovePhase::ActOffPause, POST_ACT_PAUSE_MS);
  lcd.setCursor(0, 3); lcd.print("Stopping actuator  ");
}

void MoveFSM::tick(hd44780_I2Cexp& lcd) {
  if (!active_) return;

  unsigned long now = millis();

  // Not yet time to advance — but update the countdown display if running.
  if (static_cast<long>(now - next_at_) < 0) {
    if (phase_ == MovePhase::ActOn) {
      showRemaining(lcd);
    }
    return;
  }

  switch (phase_) {
    case MovePhase::PreOn:
      // Step 2: Buck MAIN relay ON.
      digitalWrite(BUCK_MAIN_PIN, LOW);
      advanceTo(MovePhase::MainOn, MAIN_STAB_MS);
      lcd.setCursor(0, 3); lcd.print("Buck main ON...    ");
      break;

    case MovePhase::MainOn:
      // Step 3: Actuator ON.
      digitalWrite(act_pin_, LOW);
      phase_      = MovePhase::ActOn;
      started_at_ = now;
      next_at_    = now + PULSE_MS;
      lcd.setCursor(0, 3); lcd.print("Pulsing actuator   ");
      break;

    case MovePhase::ActOn:
      // Step 4: Actuator OFF.
      digitalWrite(act_pin_, HIGH);
      advanceTo(MovePhase::ActOffPause, POST_ACT_PAUSE_MS);
      lcd.setCursor(0, 3); lcd.print("Stopping actuator  ");
      break;

    case MovePhase::ActOffPause:
      // Step 5: Buck MAIN OFF.
      digitalWrite(BUCK_MAIN_PIN, HIGH);
      advanceTo(MovePhase::MainOff, BUCK_SPINDOWN_MS);
      lcd.setCursor(0, 3); lcd.print("Buck main OFF...   ");
      break;

    case MovePhase::MainOff:
      // Step 6: Buck PRE OFF.
      digitalWrite(BUCK_PRE_PIN, HIGH);
      advanceTo(MovePhase::PreOff, 0);
      lcd.setCursor(0, 3); lcd.print("Buck pre OFF       ");
      break;

    case MovePhase::PreOff:
      finish();
      lcd.setCursor(0, 3); lcd.print("Ready              ");
      break;

    case MovePhase::Idle:
      finish();
      break;
  }
}

void MoveFSM::extendTime(hd44780_I2Cexp& lcd) {
  if (phase_ != MovePhase::ActOn) return;
  next_at_ = clampEnd(next_at_, started_at_, MOVE_INCREMENT_MS, MAX_MOVE_MS);
  showRemaining(lcd, "(+30s)");
}

void MoveFSM::reduceTime(hd44780_I2Cexp& lcd) {
  if (phase_ != MovePhase::ActOn) return;

  unsigned long now = millis();
  unsigned long remain = (next_at_ > now) ? (next_at_ - now) : 0;

  if (remain <= MOVE_INCREMENT_MS) {
    stop(lcd);
  } else {
    next_at_ -= MOVE_INCREMENT_MS;
    showRemaining(lcd, "(-30s)");
  }
}

unsigned long MoveFSM::remainingMs() const {
  if (phase_ != MovePhase::ActOn) return 0;
  unsigned long now = millis();
  return (next_at_ > now) ? (next_at_ - now) : 0;
}

// --- Private helpers ---

void MoveFSM::advanceTo(MovePhase next, unsigned long delay_ms) {
  phase_   = next;
  next_at_ = millis() + delay_ms;
}

void MoveFSM::finish() {
  active_  = false;
  phase_   = MovePhase::Idle;
  act_pin_ = -1;
}

unsigned long MoveFSM::clampEnd(unsigned long end, unsigned long start,
                                unsigned long add, unsigned long max_dur) {
  unsigned long max_end = start + max_dur;
  unsigned long candidate = end + add;
  return (candidate > max_end) ? max_end : candidate;
}

void MoveFSM::showRemaining(hd44780_I2Cexp& lcd, const char* suffix) {
  unsigned long remain = remainingMs();
  unsigned long secs = (remain + 500) / 1000;
  unsigned long mm = secs / 60;
  unsigned long ss = secs % 60;

  lcd.setCursor(0, 3);
  lcd.print("Remain: ");
  if (mm < 10) lcd.print('0'); lcd.print(mm); lcd.print(':');
  if (ss < 10) lcd.print('0'); lcd.print(ss);

  if (suffix) {
    lcd.print(' '); lcd.print(suffix);
  }
  lcd.print("       ");  // clear trailing chars
}
