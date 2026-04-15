#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include "config.h"
#include "fsm.h"
#include "hardware.h"

// =====================================================================
// Auto-tracking controller.
//
// Runs every loop iteration. When master-enabled, RTC is plausible, and
// no user interaction is in progress, drives the three actuators (one at
// a time, shared buck constraint) toward a target pot reading interpolated
// linearly from the current month's configured window. Uses the existing
// MoveFSM for actual relay sequencing; this class only decides *when* and
// *which direction*, monitors the pot during a move, and calls fsm.stop()
// on arrival, stall, or emergency abort.
// =====================================================================

class AutoTrack {
public:
  enum class State : uint8_t { Off, Outside, Idle, Active };

  // Call once per loop iteration. `user_busy` should be true when the user
  // is in manual control, clock-edit, config-edit, or auto-config-edit,
  // in which case this method is a no-op.
  void tick(MoveFSM& fsm, bool user_busy, hd44780_I2Cexp& lcd);

  // Emergency stop. Halts any in-flight move immediately and flips the
  // master Enabled flag to false (persisted to NVS). The user re-enables
  // from the Auto page.
  void abort(MoveFSM& fsm, hd44780_I2Cexp& lcd);

  State    state()          const { return state_; }
  uint8_t  activeActuator() const { return active_actuator_; }  // 0..2 or 0xFF
  uint32_t msUntilNextTick() const;                              // 0 if due

private:
  State       state_            = State::Off;
  uint8_t     active_actuator_  = 0xFF;
  uint8_t     aligning_idx_     = 0;       // next actuator to evaluate (0..2)
  int         target_raw_       = 0;
  uint32_t    next_tick_ms_     = 0;
  uint32_t    last_progress_ms_ = 0;
  int         last_pot_raw_     = -1;
  uint8_t     pulse_count_      = 0;

  void scheduleNextTick();
  void startMoveIfNeeded(MoveFSM& fsm, hd44780_I2Cexp& lcd);
  void monitorActiveMove(MoveFSM& fsm, hd44780_I2Cexp& lcd);
  void finishActuator(MoveFSM& fsm, hd44780_I2Cexp& lcd);

  static bool validMonth(const MonthConfig& c);
  static bool inWindow(const MonthConfig& c, const RTC::DateTime& dt);
  static int  computeTarget(const MonthConfig& c, const RTC::DateTime& dt);
};
