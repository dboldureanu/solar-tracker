# ESP32 Pin Mapping (Quick Reference)

## GPIO Assignments

| GPIO | Function  | Connected To              | Notes        |
|------|-----------|---------------------------|--------------|
| 13   | Relay     | Actuator A - Direction 1  | Active LOW   |
| 14   | Relay     | Actuator A - Direction 2  | Active LOW   |
| 16   | Relay     | Actuator B - Direction 1  | Active LOW   |
| 17   | Relay     | Actuator B - Direction 2  | Active LOW   |
| 18   | Relay     | Actuator C - Direction 1  | Active LOW   |
| 19   | Relay     | Actuator C - Direction 2  | Active LOW   |
| 23   | Relay     | Buck MAIN relay           | Active LOW   |
| 25   | Relay     | Buck PRE-charge relay     | Active LOW   |
| 32   | Input     | Encoder A                 | INPUT_PULLUP |
| 33   | Input     | Encoder B                 | INPUT_PULLUP |
| 26   | Input     | Encoder Button            | INPUT_PULLUP |
| 34   | ADC       | Potentiometer A           | Input-only   |
| 35   | ADC       | Potentiometer B           | Input-only   |
| 36   | ADC       | Potentiometer C           | Input-only   |
| 27   | Input     | INA ALERT                 | INPUT_PULLUP |
| 21   | I2C SDA   | LCD, RTC, INA226          | Shared bus   |
| 22   | I2C SCL   | LCD, RTC, INA226          | Shared bus   |

---

## Power Pins

| Pin      | Function         | Notes                       |
|----------|------------------|-----------------------------|
| VIN / 5V | Power input     | From USB or external 5V     |
| 3V3      | Regulated output | Internal ESP32 regulator    |
| GND      | Ground           | Must be common everywhere   |

---

## Special Notes

### Input-only pins
- GPIO **34, 35, 36** cannot be used as outputs
- Used for ADC (potentiometer readings)

### Active LOW relays

All relay pins:

| Signal | Meaning       |
|--------|---------------|
| LOW    | Relay ON      |
| HIGH   | Relay OFF     |

### I2C Bus

- SDA -> GPIO 21
- SCL -> GPIO 22

Devices on the bus:
- LCD display
- RTC (DS3231)
- INA226 sensors

### Avoided Pins

Pins intentionally not used:
- GPIO 0, 2, 15 -> boot-strapping pins
- GPIO 6-11 -> used by internal flash memory
