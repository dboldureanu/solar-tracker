# Power Architecture

This document describes how power is generated, controlled, and safely delivered to the system components.

The design ensures:
- Safe startup and shutdown
- Controlled high-current switching
- Protection of both hardware and USB-connected devices

---

## Overview

The system uses a **48V battery bank** (4x 12V 200Ah lead-acid gel in series) which is stepped down by two separate converters.

```
48V Battery Bank (4x 12V 200Ah gel, series)
    |
    +---> HV Buck XL7035 (48V -> 5V, always on)
    |         |
    |         v
    |     ESP32 + sensors + LCD (logic power)
    |
    +---> Buck Converter (48V -> 12V, relay-controlled)
              |
              v
          Pre-charge Relay (GPIO 25)
              |
              v
          Main Relay (GPIO 23)
              |
              v
          Actuator Relays (GPIO 13-19)
              |
              v
          Linear Actuators
```

---

## Power Domains

### 1. High Power Domain

- **48V battery bank** (4x 12V 200Ah lead-acid gel, series)
- Nominal 48V (range ~42-56V depending on charge state)
- Feeds both buck converters

### 2. Intermediate Power (12V)

- Output of the relay-controlled buck converter
- Supplies linear actuators only
- Not always on — enabled only during movement

### 3. Logic Power (5V / 3.3V)

- **HV DC-DC buck module** (XL7035, 10-80V -> 5V, 1A max)
- Always on when battery is connected
- Powers ESP32, sensors, LCD
- ESP32 internal regulator provides 3.3V from 5V

Additional source:
- USB (development only, diode-isolated)

---

## Buck Converter Control

The buck converter is **not permanently active**.
It is enabled only when actuator movement is required.

### Control Relays

| Relay      | Function                   |
|------------|----------------------------|
| Pre-charge | Soft-start / limits inrush |
| Main       | Full power enable          |

---

## Power Sequencing (Critical)

To prevent electrical stress and ensure safe operation, the following sequence is enforced:

### Startup Sequence

1. **Pre-charge relay ON**
   - Allows gradual capacitor charging
2. Wait (~400 ms)
3. **Main relay ON**
   - Full power applied
4. Wait (~150 ms)
5. **Actuator relay ON**
   - Movement begins

### Shutdown Sequence

1. **Actuator relay OFF**
2. Wait (~100 ms)
3. **Main relay OFF**
4. Wait (~150 ms)
5. **Pre-charge relay OFF**

### Why this matters

- Prevents **inrush current spikes**
- Protects relays and buck converter
- Avoids voltage dips affecting ESP32
- Ensures longer hardware lifespan

---

## USB + External Power Coexistence

The ESP32 may be powered simultaneously by:
- USB (from laptop)
- External regulated 5V

### Problem

Without protection:
- External power can **backfeed into USB**
- Risk of damaging the laptop

### Solution: Diode Isolation

A diode is added in the USB 5V line:

```
Laptop USB 5V ──|>|──> ESP32 5V
```

- Allows power from USB -> ESP32
- Blocks power from ESP32 -> USB

### Implementation

- Diode used: **1N4007**
- Orientation:
  - Anode -> USB side
  - Cathode (stripe) -> ESP32 side

### Voltage Drop

- Typical drop: ~0.7V
- Result: ESP32 receives ~4.3V from USB
- Still within acceptable operating range

### Future Improvement

- Replace with **Schottky diode** (e.g., 1N5819)
  - Lower drop (~0.2-0.3V)
  - More efficient

---

## Grounding

All system parts must share a **common ground**:
- ESP32
- Buck converter
- Relay modules
- Sensors
- External power source

> This is essential for stable operation and correct signal reference.

---

## Safety Considerations

### DO

- Use proper voltage regulation (48V -> 12V -> 5V)
- Verify polarity before powering
- Ensure solid connections for high-current paths

### DO NOT

- Connect 12V directly to ESP32 VIN/5V
- Operate relays without proper sequencing
- Use floating grounds
- Connect USB to a system with unknown ground potential

---

## Risk Areas

- High current during actuator startup
- Relay contact wear over time
- Voltage spikes from inductive loads (actuator motors)

---

## Recommended Protections (Future)

- Flyback diodes on relay coils
- TVS diodes for surge protection
- Fuses on power lines
- Current limiting on actuator lines

---

## Notes

- Power system is designed for **intermittent operation**, not continuous load
- Buck converter should not remain powered unnecessarily
- Firmware enforces all sequencing logic
