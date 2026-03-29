# Firmware Architecture

This document describes the structure, logic, and design principles of the firmware running on the ESP32.

The firmware is responsible for:
- User interaction (encoder + LCD)
- Actuator control
- Power sequencing
- Timing and state management

---

## High-Level Structure

The firmware follows a **non-blocking, state-driven architecture**:

```
loop()
├── pollEncoder()
├── tickMoveFSM()
├── drawUI()
└── periodicTasks()
```

---

## Core Concepts

### 1. Non-blocking Execution

- Uses `millis()` instead of `delay()`
- Allows:
  - Responsive UI
  - Concurrent operations
  - Smooth timing control

### 2. Finite State Machine (FSM)

The actuator movement and power control are implemented using a **state machine**.

---

## Move FSM

### States

```
Idle
 ├─ PreOn
 ├─ MainOn
 ├─ ActOn
 ├─ ActOffPause
 ├─ MainOff
 └─ PreOff
```

### State Descriptions

| State       | Description            |
|-------------|------------------------|
| Idle        | No movement            |
| PreOn       | Pre-charge relay ON    |
| MainOn      | Main relay ON          |
| ActOn       | Actuator running       |
| ActOffPause | Short delay after stop |
| MainOff     | Main relay OFF         |
| PreOff      | Pre-charge OFF         |

### Transition Flow

```
Idle → PreOn → MainOn → ActOn → ActOffPause → MainOff → PreOff → Idle
```

### Timing Constants

| Constant           | Value    | Purpose            |
|--------------------|----------|--------------------|
| `PRECHARGE_MS`     | 400 ms   | Soft-start         |
| `MAIN_STAB_MS`    | 150 ms   | Stabilization      |
| `PULSE_MS`         | 30000 ms | Base movement time |
| `POST_ACT_PAUSE_MS`| 100 ms  | Safe stop          |
| `BUCK_SPINDOWN_MS` | 150 ms   | Power-down         |

---

## Input Handling

### Rotary Encoder

- Quadrature decoding (A/B signals)
- Accumulates steps
- Converts into actions:
  - Page navigation
  - Actuator control

### Sensitivity Control

| Mode       | Constant               | Value | Purpose                   |
|------------|------------------------|-------|---------------------------|
| Navigation | `NAV_TICKS_PER_ACTION` | 4     | Prevent oversensitivity   |
| Control    | `CTRL_TICKS_PER_PULSE` | 2     | Avoid accidental triggers |

### Button Handling

- Detects rising edge (press)
- Used for:
  - Entering control mode
  - Exiting control mode

---

## UI System

### Page-Based Design

| Page | Description |
|------|-------------|
| 0    | Clock       |
| 1    | Actuator A  |
| 2    | Actuator B  |
| 3    | Actuator C  |

### Rendering Strategy

- Only redraw when needed
- Clock updates every second
- Actuator pages update dynamically

---

## Control Mode Logic

### Entry

- Press encoder on actuator page

### Behavior

- Encoder rotation triggers movement
- Direction:
  - CW -> one direction (dir2)
  - CCW -> opposite direction (dir1)

---

## Microwave-Style Timing

### Key Idea

While an actuator is running, encoder rotation adjusts the remaining time:
- **CW** (clockwise) always **adds +30 seconds** (regardless of which direction started the move)
- **CCW** (counter-clockwise) always **subtracts -30 seconds**

### Constants

| Constant           | Value    | Purpose                |
|--------------------|----------|------------------------|
| `PULSE_MS`         | 30000 ms | Initial movement time  |
| `MOVE_INCREMENT_MS`| 30000 ms | Time added/removed     |
| `MAX_MOVE_MS`      | 60000 ms | Maximum run time cap   |

### Behavior

1. First rotation -> starts movement (30s). CW starts dir2, CCW starts dir1.
2. While running, time adjustment is always:
   - CW: extend time (+30s, capped at 60s total from start)
   - CCW: reduce time (-30s)
3. If time reaches 0 -> actuator stops immediately, FSM begins shutdown
4. Note: CW/CCW for time adjustment is independent of which direction started the move

### Runtime Adjustment

While actuator is running:
- Encoder input modifies `move.nextAt`
- Remaining time recalculated dynamically
- Display updates continuously

---

## Early Stop Logic

If user reduces time to 0:
1. Actuator stops immediately
2. FSM continues shutdown sequence safely (ActOffPause -> MainOff -> PreOff -> Idle)

---

## Relay Control Logic

- All relays are **active LOW**
- Default state = OFF (HIGH)

### Actuator Control

| dir1 | dir2 | Result      |
|------|------|-------------|
| LOW  | HIGH | Direction 1 |
| HIGH | LOW  | Direction 2 |
| HIGH | HIGH | Stop        |

> Both LOW simultaneously must never occur (short circuit risk).

---

## ADC Handling

- Uses ESP32 ADC1 (pins 34, 35, 36)
- Reads potentiometers for actuator feedback
- 12-bit resolution (0-4095)
- 11 dB attenuation (~0-3.3V range)
- Applies multi-sample averaging (12 samples) for stability

---

## Periodic Tasks

| Task                  | Interval | Condition              |
|-----------------------|----------|------------------------|
| Clock refresh         | 1 second | On clock page          |
| Potentiometer update  | ~200 ms  | On actuator page, not in control mode |

---

## Design Principles

- **Responsiveness** -> no blocking delays
- **Safety** -> enforced power sequencing
- **Predictability** -> explicit FSM states
- **Extensibility** -> easy to add features

---

## Future Improvements

- Event-driven architecture
- Task scheduling (RTOS)
- Persistent settings (EEPROM / NVS)
- Automated sun-tracking control algorithm
