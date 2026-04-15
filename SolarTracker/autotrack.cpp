#include "autotrack.h"

// ---------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------

void AutoTrack::tick(MoveFSM& fsm, bool user_busy, hd44780_I2Cexp& lcd) {
  // Highest-priority gate: if the user is actively driving the actuator
  // or navigating an edit menu, stay out of the way. Any in-flight auto
  // move should already have been cleared by abort() via the UI layer.
  if (user_busy) return;

  const SystemConfig& sys = Settings::system();

  // Master switch.
  if (!sys.auto_enabled) {
    if (state_ == State::Active) {
      fsm.stop(lcd);
      active_actuator_ = 0xFF;
    }
    state_ = State::Off;
    return;
  }

  // RTC sanity gate.
  RTC::DateTime now;
  if (!RTC::read(now) || now.year < 2024) {
    state_ = State::Outside;
    return;
  }

  // Month config.
  if (now.month < 1 || now.month > 12) {
    state_ = State::Outside;
    return;
  }
  const MonthConfig& mc = Settings::month(now.month - 1);
  if (!validMonth(mc) || !inWindow(mc, now)) {
    if (state_ == State::Active) {
      fsm.stop(lcd);
      active_actuator_ = 0xFF;
    }
    state_ = State::Outside;
    return;
  }

  target_raw_ = computeTarget(mc, now);

  // Already moving something — just monitor and decide when to stop.
  if (state_ == State::Active) {
    monitorActiveMove(fsm, lcd);
    return;
  }

  // Idle: wait until the next scheduled tick before evaluating.
  if (static_cast<int32_t>(millis() - next_tick_ms_) < 0) {
    state_ = State::Idle;
    return;
  }

  // Tick due — walk through the three actuators, align whichever is out
  // of deadband first, then park until the next tick.
  aligning_idx_ = 0;
  startMoveIfNeeded(fsm, lcd);
  if (active_actuator_ == 0xFF) {
    // All three already within deadband.
    scheduleNextTick();
    state_ = State::Idle;
  }
}

void AutoTrack::abort(MoveFSM& fsm, hd44780_I2Cexp& lcd) {
  if (fsm.isActive()) {
    fsm.stop(lcd);
  }
  active_actuator_ = 0xFF;
  state_           = State::Off;

  Settings::system().auto_enabled = false;
  Settings::save();
}

uint32_t AutoTrack::msUntilNextTick() const {
  uint32_t now = millis();
  int32_t delta = static_cast<int32_t>(next_tick_ms_ - now);
  return delta > 0 ? static_cast<uint32_t>(delta) : 0;
}

// ---------------------------------------------------------------------
// Move orchestration
// ---------------------------------------------------------------------

void AutoTrack::startMoveIfNeeded(MoveFSM& fsm, hd44780_I2Cexp& lcd) {
  // Walk remaining actuators; start the first one that needs to move.
  while (aligning_idx_ < ACTUATOR_COUNT) {
    uint8_t i = aligning_idx_;
    // Decision point: use the spike-rejecting stable read so a transient
    // doesn't spuriously trigger or skip a move.
    int current = readAdcStable(POT_PINS[i]);
    int delta   = target_raw_ - current;

    if (abs(delta) <= AUTO_DEADBAND_RAW) {
      // Already aligned; move on.
      aligning_idx_++;
      continue;
    }

    int direction_pin = (delta > 0)
        ? ACTUATOR_PINS[i].west   // need higher pot -> drive west
        : ACTUATOR_PINS[i].east;  // need lower pot  -> drive east

    fsm.start(i, direction_pin, lcd);
    active_actuator_  = i;
    pulse_count_      = 1;
    last_pot_raw_     = current;
    last_progress_ms_ = millis();
    state_            = State::Active;
    return;
  }

  // Fell through: all actuators checked, none needed movement.
  active_actuator_ = 0xFF;
}

void AutoTrack::monitorActiveMove(MoveFSM& fsm, hd44780_I2Cexp& lcd) {
  const uint8_t i = active_actuator_;
  if (i >= ACTUATOR_COUNT) {
    state_ = State::Idle;
    return;
  }

  // If the FSM has spun back down (pulse timed out), decide whether we
  // need another pulse or this actuator is done.
  if (!fsm.isActive()) {
    // Decision point between pulses — use the stable read.
    int current = readAdcStable(POT_PINS[i]);
    int delta   = target_raw_ - current;

    if (abs(delta) <= AUTO_DEADBAND_RAW) {
      finishActuator(fsm, lcd);
      return;
    }

    if (pulse_count_ >= AUTO_MAX_PULSES) {
      // Ran out of chained pulses; give up on this actuator this pass.
      finishActuator(fsm, lcd);
      return;
    }

    // Start another pulse in the appropriate direction.
    int direction_pin = (delta > 0)
        ? ACTUATOR_PINS[i].west
        : ACTUATOR_PINS[i].east;

    fsm.start(i, direction_pin, lcd);
    pulse_count_++;
    last_pot_raw_     = current;
    last_progress_ms_ = millis();
    return;
  }

  // FSM still in a phase — only meaningful to watch pot during ActOn.
  if (fsm.phase() != MovePhase::ActOn) return;

  // Fast polling during active travel (readAdcAvg spans ~20 ms, fast enough
  // to stop within ~1 count of target even at full actuator speed).
  int current = readAdcAvg(POT_PINS[i]);
  int delta   = target_raw_ - current;

  if (abs(delta) <= AUTO_DEADBAND_RAW) {
    // Candidate stop: confirm with a spike-rejecting stable read before
    // committing. One 5-count noise dip into deadband shouldn't stop us
    // short of target.
    int confirm = readAdcStable(POT_PINS[i]);
    if (abs(target_raw_ - confirm) <= AUTO_DEADBAND_RAW) {
      fsm.stop(lcd);
      finishActuator(fsm, lcd);
      return;
    }
  }

  // Stall detection uses the fast read: a noise spike showing zero progress
  // for a single sample is not a stall, the window is 5 seconds wide.
  if (abs(current - last_pot_raw_) >= AUTO_STALL_DELTA_RAW) {
    last_pot_raw_     = current;
    last_progress_ms_ = millis();
  } else if (millis() - last_progress_ms_ >= AUTO_STALL_WINDOW_MS) {
    fsm.stop(lcd);
    finishActuator(fsm, lcd);  // skip this actuator this pass
  }
}

void AutoTrack::finishActuator(MoveFSM& fsm, hd44780_I2Cexp& lcd) {
  active_actuator_ = 0xFF;
  aligning_idx_++;
  pulse_count_ = 0;

  // Try next actuator.
  startMoveIfNeeded(fsm, lcd);

  if (active_actuator_ == 0xFF) {
    // No further actuator needs work this pass.
    scheduleNextTick();
    state_ = State::Idle;
  }
}

// ---------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------

void AutoTrack::scheduleNextTick() {
  const SystemConfig& sys = Settings::system();
  uint32_t tick = static_cast<uint32_t>(sys.tick_min) * 60UL * 1000UL;
  next_tick_ms_ = millis() + tick;
}

bool AutoTrack::validMonth(const MonthConfig& c) {
  if (c.start_raw >= c.end_raw) return false;
  int s = c.start_h * 60 + c.start_m;
  int e = c.end_h   * 60 + c.end_m;
  return s < e;
}

bool AutoTrack::inWindow(const MonthConfig& c, const RTC::DateTime& dt) {
  int now_min = dt.hour    * 60 + dt.minute;
  int s       = c.start_h  * 60 + c.start_m;
  int e       = c.end_h    * 60 + c.end_m;
  return now_min >= s && now_min <= e;
}

int AutoTrack::computeTarget(const MonthConfig& c, const RTC::DateTime& dt) {
  int now_sec = dt.hour    * 3600 + dt.minute  * 60 + dt.second;
  int s_sec   = c.start_h  * 3600 + c.start_m  * 60;
  int e_sec   = c.end_h    * 3600 + c.end_m    * 60;

  if (now_sec <= s_sec) return c.start_raw;
  if (now_sec >= e_sec) return c.end_raw;

  long span    = e_sec - s_sec;
  long progr   = now_sec - s_sec;
  long range   = static_cast<long>(c.end_raw) - static_cast<long>(c.start_raw);
  return static_cast<int>(c.start_raw + (range * progr) / span);
}
