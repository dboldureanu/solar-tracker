# System Overview

This document describes the overall architecture and purpose of the Solar Tracker system, including its main components and how they interact.

---

## Purpose

The system is designed to control the position of **9 solar panels** (arranged in 3 rows of 3) using **three linear actuators** — one per row — for **azimuth tracking** (east-to-west sun following). Tilt is adjusted manually.

The installation is located in **Chisinau, Moldova** (latitude 47.0°N, longitude 28.8°E).

The primary goals are:
- Reliable actuator control
- Safe power management
- Simple and intuitive user interaction
- Clock-based automatic sun tracking

---

## High-Level Architecture

The system is composed of the following main subsystems:

```
        +---------------------+
        |     User Input      |
        |  (Rotary Encoder)   |
        +----------+----------+
                   |
                   v
        +---------------------+
        |      ESP32 MCU      |
        |   (Control Logic)   |
        +----+----+----+------+
             |    |    |
             |    |    |
 +-----------+    |    +-----------+
 |                |                |
 v                v                v
+---------+ +-----------+ +-------------+
|   LCD   | |    RTC    | |   Sensors   |
|  (I2C)  | |  DS3231   | |   INA226    |
+---------+ +-----------+ +-------------+
                  |
                  v
        +-------------------+
        |   Relay Control   |
        +---------+---------+
                  |
                  v
        +-------------------+
        |  Linear Actuators |
        |   (3x Motors)     |
        +-------------------+

                  ^
                  |
        +-------------------+
        |   Power System    |
        |  (Buck + Relays)  |
        +-------------------+
```

---

## Subsystems Description

### 1. Control Unit (ESP32)

The **ESP32 DevKitC** acts as the central controller:
- Handles user input (encoder)
- Drives relays
- Manages UI (LCD)
- Communicates with RTC and sensors via I2C
- Executes control logic using a **non-blocking FSM**

---

### 2. User Interface

**Components:**
- Rotary encoder (with push button)
- I2C LCD display (20x4 characters)

**Responsibilities:**
- Page navigation
- Enter/exit control mode
- Display system status (time, actuator info, timers)
- Provide feedback during actuator movement

---

### 3. Actuation System

- **9 solar panels** in 3 rows of 3 (each panel ~60 x 120 cm)
- 3 independent **linear actuators** (one per row, azimuth only)
- Each actuator: 450 mm stroke, 12V, 900N thrust, ~5 mm/s speed (~90s full travel)
- Each actuator controlled by **2 relays** (direction control)
- Only one direction active at a time
- Built-in 10 kOhm potentiometer for position feedback

**Behavior:**
- Controlled in timed pulses
- Direction selected via encoder rotation
- Duration adjustable dynamically (+/-30s steps)

**Tracking strategy:**
- Primary: clock-based positioning (calculate sun azimuth from time + Chisinau coordinates)
- Secondary: potentiometer feedback for closed-loop position verification
- Fallback: timed movement (actuator speed is ~5 mm/s, determined experimentally)

---

### 4. Power System

The actuators are powered through a controlled sequence:

```
48V Source
    |
    v
Buck Converter (48V -> 12V)
    |
    v
Pre-charge Relay
    |
    v
Main Relay
    |
    v
Actuator Relays
    |
    v
Linear Actuators
```

**Key concept:**
- The buck converter is **not always on**
- It is enabled only when movement is required
- Controlled via **two relays (PRE + MAIN)**

---

### 5. Sensors

**RTC (DS3231)**
- Keeps track of date and time
- Battery-backed
- Used for display and future automation

**INA226 (optional / planned)**
- Measures voltage, current, power
- Enables monitoring and diagnostics

**Potentiometers**
- Provide analog feedback
- Used for actuator calibration

---

## Software Architecture

The firmware is designed around:

### 1. State-based control (FSM)
Handles:
- Power sequencing
- Actuator movement lifecycle
- Safe transitions

### 2. Non-blocking execution
- Uses `millis()` instead of delays
- Allows UI + control to run simultaneously

### 3. Page-based UI
- 4 pages: Clock, Actuator A, Actuator B, Actuator C

---

## Typical Operation Flow

1. System boots -> displays clock page
2. User rotates encoder -> switches pages
3. User selects actuator page
4. User presses button -> enters control mode
5. User rotates encoder -> actuator starts moving
6. System:
   - Powers buck (PRE -> MAIN)
   - Activates actuator relay
   - Runs for defined duration
7. User can:
   - Increase time (+30s)
   - Decrease time (-30s)
   - Stop movement early
8. System safely powers down:
   - Actuator OFF -> MAIN OFF -> PRE OFF

---

## Design Principles

- **Safety first** -> controlled power sequencing
- **Non-blocking logic** -> responsive UI
- **Modularity** -> easy to extend
- **Clarity** -> explicit states and transitions

---

## Notes

- Only one actuator is controlled at a time
- Relays are **active LOW**
- USB + external power coexist with proper isolation
- Manual control is working; clock-based automatic tracking is the next step
