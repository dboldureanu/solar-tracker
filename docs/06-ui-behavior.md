# UI Behavior

This document describes how the user interacts with the system via the **rotary encoder and LCD**, including navigation, control modes, and on-screen feedback.

---

## Navigation Model

The UI is based on a **page system**, controlled by the rotary encoder.

### Pages

| Page Index | Name       | Description        |
|------------|------------|--------------------|
| 0          | Clock      | Displays date/time |
| 1          | Actuator A | Control page       |
| 2          | Actuator B | Control page       |
| 3          | Actuator C | Control page       |

---

## Page Navigation

### Rotation (Normal Mode)

- Rotate encoder:
  - Clockwise -> next page
  - Counter-clockwise -> previous page

### Sensitivity

- Navigation requires multiple encoder ticks
- Controlled by `NAV_TICKS_PER_ACTION` (currently 4)
- Prevents accidental page switching

---

## Button Behavior

| Page           | Button Action                |
|----------------|------------------------------|
| Clock          | No action                    |
| Actuator pages | Enter / Exit control mode    |

---

## Control Mode

### Entering Control Mode

1. Navigate to an actuator page
2. Press encoder button

-> System enters **control mode**

### Exiting Control Mode

- Press encoder button again
- Returns to normal navigation mode
- If a move is in progress, the FSM continues its shutdown safely

---

## Control Mode Behavior

While in control mode:

### Encoder Rotation

| Direction         | Action                       |
|-------------------|------------------------------|
| Clockwise         | Move actuator (dir2)         |
| Counter-clockwise | Move actuator (dir1)         |

---

## Microwave-Style Timing

The system uses a **cumulative timing model**, similar to a microwave.

### Behavior

- First rotation:
  - Starts actuator movement
  - Sets initial time = **30 seconds**
- Additional rotations (while running):
  - CW always adds **+30 seconds** (capped at 60s total from start)
  - CCW always subtracts **-30 seconds**
  - Note: this is independent of which direction started the move

### Examples

| Action           | Result              |
|------------------|---------------------|
| Rotate CW once   | Start 30s           |
| Rotate CW again  | 60s total (max cap) |
| Rotate CCW once  | Back to 30s         |
| Reduce to 0      | Movement stops      |

---

## Stop Behavior

- If remaining time reaches **0**:
  - Actuator stops immediately
  - Power-down sequence begins (ActOffPause -> MainOff -> PreOff -> Idle)

---

## Display Layouts

### 1. Clock Page

```
Date & Time
YYYY-MM-DD
HH:MM:SS
Rotate: pages | Btn: -
```

- Updates every second
- Button has no effect

### 2. Actuator Page (Normal Mode)

```
Actuator A
POT: 2048  1.65V
Rotate: next/prev
Btn: Control mode
```

- Shows actuator name
- Potentiometer value (raw ADC + voltage)

### 3. Actuator Page (Control Mode - Idle)

```
Actuator A
POT: 2048  1.65V
CONTROL: rotate =>
Dir: CCW<-  ->CW
```

- Waiting for user input
- No movement active

### 4. Actuator Page (Running)

```
Actuator A
POT: 2048  1.65V
CONTROL: rotate =>
Remain: 00:25
```

- Displays remaining time as MM:SS
- Updates continuously

### 5. Time Adjustment Feedback

When adjusting time while running:

- Adding time:
  ```
  Remain: 01:00 (+30s)
  ```
- Reducing time:
  ```
  Remain: 00:30 (-30s)
  ```

---

## Real-Time Updates

| Element          | Update Rate |
|------------------|-------------|
| Clock            | 1 second    |
| Potentiometer    | ~200 ms     |
| Remaining time   | Continuous  |

---

## Interaction Summary

### Normal Mode

- Rotate -> change page
- Button -> enter control (only on actuator pages)

### Control Mode

- Rotate -> control actuator (direction + time)
- Button -> exit control mode

---

## Important Behavior Rules

- Only one actuator can be controlled at a time
- Control mode disables page navigation
- Movement continues even if UI updates
- FSM handles safe power transitions independently

---

## UX Notes

- Encoder sensitivity intentionally reduced to avoid accidental movements
- Visual feedback always matches system state

---

## Possible UI Improvements

- Add icons or arrows for direction
- Display actuator position as percentage
- Add warnings (overcurrent, limits)
- Add confirmation before movement
- Add menu/settings page
