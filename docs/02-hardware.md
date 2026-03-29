# Hardware Components

This document describes all physical components used in the Solar Tracker system, including control boards, actuators, power elements, and interface devices.

---

## Control Unit

### ESP32 DevKitC v4

- Main microcontroller of the system
- Dual-core processor with WiFi/Bluetooth capability
- Operates at 3.3V logic level
- Provides:
  - GPIO control for relays
  - ADC inputs for potentiometers
  - I2C communication (RTC, LCD, sensors)
  - Encoder input handling

---

## User Interface

### I2C LCD Display

- **Model:** LCD2004, 20x4 character LCD, [AliExpress listing](https://aliexpress.ru/item/1005006831710761.html)
- Interface: I2C via PCF8574 backpack (auto-detected by `hd44780_I2Cexp` driver)
- I2C address: auto-detected (typically 0x27 or 0x3F)

| Parameter            | Value                                   |
|----------------------|-----------------------------------------|
| Resolution           | 20x4 characters                         |
| Controller chip      | SPLC780C / HD44780 compatible           |
| Operating voltage    | 3.3V                                    |
| Power consumption    | 40 mA                                   |
| Display type         | STN/FSTN                                |
| Backlight            | Blue (white on blue) or Yellow-green    |
| Display area         | 70.1 x 20.76 mm                        |
| Module dimensions    | 98 x 60 x 14 mm                        |
| Character size       | 2.94 x 4.74 mm                         |
| Operating temperature| -20 to +70 C                            |

- Used for:
  - Displaying system status
  - Navigation feedback
  - Actuator control information

### Rotary Encoder (with push button)

- Model: **EC11** (Stlxy), [AliExpress listing](https://aliexpress.ru/item/10000056483250.html)
- Type: Incremental rotary encoder (A/B quadrature signals)
- 20 detent positions, 360-degree rotation
- Integrated momentary push button
- 5-pin (A, B, Button, GND, GND)
- Used for:
  - Page navigation
  - Entering/exiting control mode
  - Adjusting actuator movement duration

---

## Actuation System

### Linear Actuators (x3)

- Used to physically move/adjust the solar panel
- Each actuator supports:
  - Bidirectional movement
  - Controlled via polarity switching (relays)
  - Built-in potentiometer for position feedback
  - Built-in end-stop limit switches (both ends)

**Model:** XINHUANGDUO BHTGA-DW ([AliExpress listing](https://aliexpress.ru/item/1021789697.html))

| Parameter                  | Value                          |
|----------------------------|--------------------------------|
| Stroke                     | 450 mm                         |
| Operating voltage           | 12V DC                         |
| Max current                 | 2.5 A                          |
| Max push force (thrust)     | 900 N                          |
| Max pull force              | 750 N                          |
| Static load (forced)        | 950 N                          |
| Max load                    | 1500 N                         |
| Self-locking force          | 2300 N                         |
| Feedback potentiometer      | Linear (1k / 5k / 10k options) |
| End-stop limit switches     | Built-in (both ends)           |
| Speed (approx)              | ~5 mm/s (~90s full stroke)     |
| Power                       | 20W                            |
| Motor type                  | Brushed, permanent magnet      |
| Protection rating           | IP65 (IEC-60529)               |
| Operating temperature       | -26 to +65 C                   |
| Operating humidity           | 5-95% (non-condensing)         |
| Duty cycle                  | 20%                            |
| Lifespan                    | 50,000 full cycles             |
| Construction                | Aluminum alloy tube/rods, zinc alloy housing, powder metal gears |

### Relay Modules

- Total relays used: **8** (6-channel module for actuators + 2-channel module for buck control)
- Brand: **Liludin**, optocoupler-isolated, low-level trigger
- [AliExpress listing](https://aliexpress.ru/item/1005004651717236.html)

| Parameter            | Value                        |
|----------------------|------------------------------|
| Trigger level        | Active LOW (0-2V for 5V version) |
| Trigger current      | 2 mA                         |
| Quiescent current    | 4 mA (5V version)            |
| Operating current    | 65 mA (5V version)           |
| Contact rating       | 250V 10A AC / 30V 10A DC     |
| Isolation            | Optocoupler                  |
| Operating temperature| -45 to +85 C                 |
| Mounting             | 3.1 mm bolt holes            |

**Allocation:**

| Function         | GPIO   | Description         |
|------------------|--------|---------------------|
| Actuator A       | 13, 14 | Direction control   |
| Actuator B       | 16, 17 | Direction control   |
| Actuator C       | 18, 19 | Direction control   |
| Buck Main        | 23     | Enables main power  |
| Buck Pre-charge  | 25     | Soft-start          |

**Notes:**
- Each actuator uses **2 relays** (forward/reverse)
- Only one direction must be active at a time
- All relays default to **OFF (HIGH)**
- Each relay has NO (normally open), NC (normally closed), and COM terminals

---

## Power System

### Primary Source

- **48V battery bank**: 4x 12V 200Ah lead-acid gel batteries in series
- Nominal voltage: 48V (range ~42-56V depending on charge state)

### ESP32 Power: HV DC-DC Buck (48V -> 5V)

Powers the ESP32 and logic-level components (always on).

- **Chip:** XL7035 (replaces XL7015 / LM2596HV)
- **Brand:** eletechsup, [AliExpress listing](https://aliexpress.ru/item/32824319280.html)

| Parameter                | Value               |
|--------------------------|---------------------|
| Input voltage            | 10-80V DC           |
| Output voltage           | 5V DC (fixed)       |
| Max output current       | 1A                  |
| Switching frequency      | 150 kHz             |
| Max efficiency           | 85%                 |
| Quiescent current        | ~2.5 mA             |
| Operating temperature    | -40 to +85 C        |
| Dimensions               | 59 x 30 x 16 mm     |

**Protections:** reverse polarity input, overvoltage (TVS), overcurrent, thermal shutdown, short circuit.

### Actuator Power: Buck Converter (48V -> 12V)

Powers the linear actuators. **Not always on** — enabled only when movement is required, controlled via two relays with safe power sequencing.

- **Type:** 200W synchronous buck module (non-isolated), [AliExpress listing](https://aliexpress.ru/item/1005010087482983.html)
- Output voltage adjustable via potentiometer (default 12V)

| Parameter                | Value                            |
|--------------------------|----------------------------------|
| Input voltage            | 8-55V DC (abs max 60V)          |
| Output voltage           | 1-36V DC adjustable (set to 12V)|
| Output current           | 10A continuous, 15A peak         |
| Output power             | 100W natural cooling, 200W with enhanced cooling |
| Switching frequency      | 180 kHz                          |
| Max efficiency           | 94%                              |
| Operating temperature    | -10 to +85 C                     |
| Dimensions               | 70 x 38 x 31 mm (aluminum case) |
| Connection               | Screw terminals (no soldering)   |

**Protections:** overcurrent (>15A), reverse polarity input.

### Buck Control Relays

Two relays are used to safely control the actuator buck converter:

1. **Pre-charge relay** (GPIO 25)
   - Allows gradual energizing
   - Reduces inrush current

2. **Main relay** (GPIO 23)
   - Supplies full power after stabilization

### USB Power (Development)

- ESP32 can also be powered via USB
- Used for programming and serial debugging

**Isolation:**
- USB 5V line is **diode-isolated** (1N4007)
- Prevents backfeeding into the laptop

---

## Sensors

### DS3231 RTC Module

- **Chip:** DS3231SN with AT24C32 EEPROM (32Kbit)
- **Brand:** Liludin, [AliExpress listing](https://aliexpress.ru/item/1005001875764383.html)
- Module: DS3231 mini module (3.3V/5V compatible)
- Battery: LIR2032 rechargeable (keeps clock running when main power is off)
- Communicates via I2C (RTC address 0x68, EEPROM address 0x57 — adjustable via A0/A1/A2)
- I2C max speed: 400 kHz at 5V

| Parameter            | Value                          |
|----------------------|--------------------------------|
| Operating voltage    | 3.3-5.5V                       |
| Clock accuracy       | +/-2 ppm (0-40 C range, ~1 min/year) |
| Temperature sensor   | Built-in, +/-3 C accuracy      |
| EEPROM               | AT24C32, 32Kbit                |
| Alarms               | 2 configurable                 |
| Square wave output   | Programmable                   |
| Calendar valid until | Year 2100 (with leap year compensation) |
| Dimensions           | 38 x 22 x 14 mm               |

### INA226 Current/Voltage Sensors (optional)

- **Chip:** INA226, **Brand:** diymore, [AliExpress listing](https://aliexpress.ru/item/1005006460678073.html)
- I2C current/voltage/power monitor with shunt resistor

| Parameter              | Value                            |
|------------------------|----------------------------------|
| Bus voltage range      | 0-36V DC                         |
| Current range          | -20A to +20A (bidirectional)     |
| Supply voltage         | 2.7-5.5V                        |
| Resolution             | 16-bit (1.25 mA current, 1.25 mV voltage) |
| Accuracy               | 1%                               |
| Power consumption      | ~330 uA (typ), <1 mA            |
| Interface              | I2C (up to 16 addresses via A0/A1 solder jumpers) |
| Alert output           | Programmable alarm pin           |
| Operating temperature  | -40 to +125 C                    |
| Dimensions             | 20 x 27 mm                       |

- Used for monitoring actuator current draw and system diagnostics
- Connected to ESP32 INA ALERT pin (GPIO 27) for alarm notifications

### Potentiometers (x3)

- **Built into the linear actuators** (10 kOhm linear)
- Connected to ESP32 ADC1 pins (GPIO 34, 35, 36)
- Provide analog position feedback for each actuator
- Used for calibration and position estimation

**Signal conditioning circuit (per channel):**

```
Actuator pot (0-5V) ──[ 1kΩ series ]──┬── ESP32 ADC pin (0-3.3V)
                                       |
                                  [ 100nF ]
                                       |
                                      GND
                                       |
                                  [ 10kΩ pull-down ]
                                       |
                                      GND
```

- **1 kOhm series resistor**: limits current into ADC, protects ESP32
- **100 nF capacitor to GND**: low-pass RC filter (RC = ~100 us), smooths voltage spikes and mechanical noise
- **10 kOhm pull-down**: prevents floating voltage at mid-stroke
- Note: a voltage divider may be needed to scale 5V pot output to 3.3V ADC range

---

## Passive Components

### Diodes

- Used for power isolation
- Current: **1N4007**
  - Prevents reverse current into USB
  - Installed inline on USB 5V (VCC)

> For better efficiency, Schottky diodes (e.g., 1N5819) are recommended in future revisions.

---

## Connectivity

### I2C Bus

Shared by: LCD, RTC, INA226 sensors

### GPIO

Used for: relay control, encoder signals, button input

---

## Important Notes

- All grounds (**GND**) must be common across: ESP32, power system, sensors
- Relays are **active LOW**
- External power must be properly regulated before reaching the ESP32

---

## Future Hardware Improvements

- Replace mechanical relays with MOSFET-based drivers
- Add end-stop sensors for actuators
- Add hardware protection (fuses, TVS diodes)
- Improve power isolation (ideal diode or power mux)
