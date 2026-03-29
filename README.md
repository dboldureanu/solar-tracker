# Solar Tracker Project

A custom-built solar tracking system powered by an ESP32, designed to control multiple linear actuators with a user-friendly interface and safe power management.

---

## Overview

This project controls **three linear actuators** used to adjust the position of a solar panel system.
It features a **rotary encoder + LCD interface**, allowing manual control, monitoring, and future automation.

The system is designed with:
- Safe **power sequencing** (buck converter + relays)
- Modular **hardware architecture**
- Expandable **firmware (FSM-based design)**

---

## Features

- Real-time clock (RTC) display
- Rotary encoder navigation (pages + control mode)
- Safe actuator control via relay sequencing
- Buck converter pre-charge + main relay logic
- Microwave-style timing control (+30s / -30s)
- LCD interface (20x4 I2C)
- Potentiometer feedback for calibration
- Expandable for sensors (INA226 monitoring)

---

## System Architecture

The system is divided into several subsystems:

- **Control Unit** -> ESP32 (main logic)
- **User Interface** -> LCD + rotary encoder
- **Power System** -> 48V -> buck converter -> relays
- **Actuation** -> 3x linear actuators
- **Sensors** -> RTC + optional current/voltage monitors

---

## Documentation

Full project documentation is in the `docs/` folder:

| File | Topic |
|------|-------|
| [01-overview.md](docs/01-overview.md) | System architecture and purpose |
| [02-hardware.md](docs/02-hardware.md) | Hardware components |
| [03-wiring.md](docs/03-wiring.md) | Wiring and pin mapping (single source of truth) |
| [04-power-architecture.md](docs/04-power-architecture.md) | Power design and sequencing |
| [05-firmware.md](docs/05-firmware.md) | Firmware architecture and FSM |
| [06-ui-behavior.md](docs/06-ui-behavior.md) | UI behavior and interaction |
| [pinout.md](docs/pinout.md) | Quick-reference pin table |

---

## Getting Started

### Requirements

- ESP32 DevKit board (ESP32-WROOM-32 / 32D, 38-pin)
- Arduino IDE
- USB cable (Type-C or Micro-B depending on board)

### Arduino IDE Setup

#### 1. Install ESP32 board support

1. **File** -> **Preferences**
2. In **"Additional Board Manager URLs"**, paste:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
   (If you already have other URLs, separate them with a comma)
3. Click **OK**
4. **Tools** -> **Board** -> **Boards Manager**
5. Search for **"esp32"**, install **"ESP32 by Espressif Systems"**

#### 2. Install required libraries

- **Tools** -> **Manage Libraries**, then search and install:
  - **hd44780** by Bill Perry (LCD driver)
- `Wire` is built-in, no installation needed

#### 3. Board settings

- **Tools** -> **Board** -> select **"ESP32 Dev Module"**
- **Tools** -> **Port** -> select the COM port your ESP32 is connected to
- All other settings can stay at defaults

### Building and uploading

1. Open `SolarTracker/SolarTracker.ino` in Arduino IDE
2. Click **Verify** (checkmark) to compile without uploading
3. Connect ESP32 via USB
4. Click **Upload** (arrow) to flash the firmware
5. **Tools** -> **Serial Monitor** at 115200 baud (optional, for debug output)

### Project directories

- `SolarTracker/` — current clean firmware (modular, multi-file)
- `SolarTracker/SolarTrackerCalibrationAndTesting.ino.bak` — original single-file firmware (reference only)

---

## Safety Notes

- Never connect **12V or higher directly to ESP32 VIN**
- Ensure **common ground** between all power sources
- Use proper **relay sequencing** (handled in firmware)
- USB + external power should be **isolated with a diode**

---

## Future Improvements

- Automatic sun tracking algorithm (clock-based scheduling)
- WiFi / remote control
- Data logging
- Closed-loop actuator positioning
- Safety limits and fault detection

---

## License

TBD
