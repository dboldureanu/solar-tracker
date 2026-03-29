# Wiring & Pin Mapping

This document defines all electrical connections between the ESP32 and the system components.
It is the **single source of truth** for pin assignments and wiring.

---

## ESP32 Pin Mapping

### I2C Bus

| Function | GPIO | Notes      |
|----------|------|------------|
| SDA      | 21   | Shared bus |
| SCL      | 22   | Shared bus |

Connected devices:
- LCD (I2C)
- RTC (DS3231)
- INA226 sensors

---

### Actuator Relays

Each actuator is controlled by **2 relays** (direction control).

| Actuator | Direction | GPIO |
|----------|-----------|------|
| A        | dir1      | 13   |
| A        | dir2      | 14   |
| B        | dir1      | 16   |
| B        | dir2      | 17   |
| C        | dir1      | 18   |
| C        | dir2      | 19   |

> Only one direction relay must be ON at a time.

---

### Buck Converter Control

| Function         | GPIO | Description        |
|------------------|------|--------------------|
| Pre-charge relay | 25   | Enables soft-start |
| Main relay       | 23   | Enables full power |

---

### Rotary Encoder

| Signal    | GPIO | Notes        |
|-----------|------|--------------|
| Encoder A | 32   | INPUT_PULLUP |
| Encoder B | 33   | INPUT_PULLUP |
| Button    | 26   | INPUT_PULLUP |

---

### Potentiometers (ADC)

| Potentiometer | GPIO | ADC Channel |
|---------------|------|-------------|
| Actuator A    | 34   | ADC1        |
| Actuator B    | 35   | ADC1        |
| Actuator C    | 36   | ADC1        |

> These are **input-only pins** (no digital output).

---

### Other Signals

| Function  | GPIO | Notes        |
|-----------|------|--------------|
| INA ALERT | 27   | INPUT_PULLUP |

---

## Wiring Overview

### I2C Connections

All I2C devices share the same lines:

```
ESP32 GPIO21 (SDA) ─────┬── LCD
                         ├── RTC (DS3231)
                         └── INA226 modules

ESP32 GPIO22 (SCL) ─────┬── LCD
                         ├── RTC
                         └── INA226 modules
```

> Pull-up resistors are typically included on modules.

---

### Relay Wiring (Logic Side)

- Relay modules connect directly to ESP32 GPIOs
- Logic is **active LOW**:

| Signal | State | Meaning        |
|--------|-------|----------------|
| LOW    | ON    | Relay activated |
| HIGH   | OFF   | Relay inactive  |

---

### Actuator Control Logic

Each actuator uses two relays:

```
dir1 = LOW,  dir2 = HIGH  ->  move one direction
dir1 = HIGH, dir2 = LOW   ->  move opposite direction
dir1 = HIGH, dir2 = HIGH  ->  STOP
```

> Never set both LOW simultaneously (short circuit risk).

---

### Buck Relay Wiring

```
ESP32 GPIO25  ->  Pre-charge relay
ESP32 GPIO23  ->  Main relay
```

Sequence:
1. PRE ON
2. MAIN ON
3. Actuator ON
4. Actuator OFF
5. MAIN OFF
6. PRE OFF

---

## Power Connections

### ESP32 Power

- Powered via:
  - USB (development)
  - OR external 5V (via VIN/5V pin)

### External Power System

```
48V Source
    |
    v
Buck Converter (48V -> 12V)
    |
    v
Relays (PRE + MAIN)
    |
    v
Actuators
```

### USB Power Isolation

USB 5V line modified with diode:

```
USB 5V ──|>|──> ESP32 5V
```

Prevents backfeeding into laptop.

---

## Critical Rules

### 1. Common Ground

All components MUST share GND:
- ESP32
- Buck converter
- Relays
- Sensors

### 2. Voltage Limits

- ESP32 GPIO: **3.3V only**
- VIN/5V pin: **5V only**
- NEVER connect 12V directly to ESP32

### 3. Relay Safety

- Default state must be OFF (HIGH)
- Avoid switching directions instantly
- Ensure proper sequencing

### 4. Encoder Stability

- Use INPUT_PULLUP
- Keep wires short
- Avoid noise near relay lines

---

## Debug Tips

- If LCD not detected -> check I2C address (use I2C scanner sketch)
- If relays inverted -> logic is active LOW, check wiring
- If encoder unstable -> check grounding and debounce
- If ADC noisy -> average readings in software (firmware uses 12-sample average)

---

## Notes

- Pin assignments are chosen to:
  - Avoid boot-strapping pins (GPIO 0, 2, 15)
  - Avoid flash memory pins (GPIO 6-11)
  - Use ADC1 channels (safe when WiFi is active)
  - Keep I2C on default pins (21, 22)
