#pragma once
#include <Arduino.h>
#include "config.h"

// Forward declaration for LCD status updates
class hd44780_I2Cexp;

enum class MovePhase : uint8_t {
  Idle,
  PreOn,
  MainOn,
  ActOn,
  ActOffPause,
  MainOff,
  PreOff,
};

class MoveFSM {
public:
  // Start a new move. Does nothing if already active.
  void start(uint8_t actuatorIndex, int directionPin, hd44780_I2Cexp& lcd);

  // Stop the actuator immediately and begin shutdown sequence.
  void stop(hd44780_I2Cexp& lcd);

  // Call every loop iteration. Advances state when timers expire.
  // Pass LCD pointer for status line updates (line 3).
  void tick(hd44780_I2Cexp& lcd);

  // Extend the running time by MOVE_INCREMENT_MS (capped at MAX_MOVE_MS).
  void extendTime(hd44780_I2Cexp& lcd);

  // Reduce the running time by MOVE_INCREMENT_MS. Stops if time runs out.
  void reduceTime(hd44780_I2Cexp& lcd);

  bool       isActive()   const { return active_; }
  MovePhase  phase()      const { return phase_; }
  unsigned long remainingMs() const;

private:
  bool          active_    = false;
  MovePhase     phase_     = MovePhase::Idle;
  uint8_t       actuator_  = 0;
  int           act_pin_   = -1;
  unsigned long started_at_ = 0;  // when ActOn began
  unsigned long next_at_    = 0;  // when to advance to next phase

  void advanceTo(MovePhase next, unsigned long delay_ms);
  void finish();

  // Clamp end time so total duration doesn't exceed MAX_MOVE_MS.
  static unsigned long clampEnd(unsigned long end, unsigned long start,
                                unsigned long add, unsigned long max_dur);

  // Write the remaining time to LCD line 3.
  void showRemaining(hd44780_I2Cexp& lcd, const char* suffix = nullptr);
};
