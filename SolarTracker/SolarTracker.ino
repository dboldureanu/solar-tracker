#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

#include "config.h"
#include "hardware.h"
#include "fsm.h"
#include "autotrack.h"
#include "ui.h"

// Global objects
hd44780_I2Cexp lcd;
Encoder encoder;
MoveFSM fsm;
AutoTrack autotrack;
UI ui;

void setup() {
  Serial.begin(115200);

  // Long settle delay at the very top of setup(). Wide-input switching
  // converters (XL7035 / LM2596HV clones) have slow soft-start and poor
  // transient response at cold boot; giving the 5 V rail a full second
  // to stabilize before we start drawing current avoids brown-out loops
  // and marginal I2C init.
  delay(1000);

  // GPIO inputs first — no current draw, harmless at any rail state.
  encoder.init();
  pinMode(INA_ALERT_PIN, INPUT_PULLUP);

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setTimeOut(50);

  // Let the LCD's internal HD44780 power-on reset finish before we start
  // talking to it (datasheet spec ~40 ms; we give it 200 for margin).
  delay(200);

  // LCD init with one retry — hd44780 cold-boot occasionally fails on the
  // first attempt after a marginal power rail and succeeds the second.
  int lcdStatus = lcd.begin(LCD_COLS, LCD_ROWS);
  if (lcdStatus) {
    Serial.print(F("LCD init failed: ")); Serial.println(-lcdStatus);
    delay(500);
    lcdStatus = lcd.begin(LCD_COLS, LCD_ROWS);
    if (lcdStatus) {
      Serial.print(F("LCD init retry failed: ")); Serial.println(-lcdStatus);
    } else {
      Serial.println(F("LCD init recovered on retry."));
    }
  }
  lcd.backlight();

  // Relay init AFTER LCD, matching the legacy firmware order. Keeping this
  // late means the 8 relay GPIOs stay in high-Z (relay modules pull them
  // high via their onboard resistors) until after we've successfully
  // talked to the LCD — fewer simultaneous current draws on the 5 V rail
  // during the critical I2C init window.
  initRelays();

  initAdc();
  Settings::load();

  if (RTC::isPresent()) {
    // Only seed the RTC if it looks uninitialized (dead coin cell / first boot).
    // Otherwise the set-time menu would be clobbered on every reboot.
    RTC::DateTime dt;
    if (!RTC::read(dt) || dt.year < 2024) {
      RTC::setFromCompileTime();
      Serial.println(F("RTC uninitialized; seeded from compile time."));
    } else {
      Serial.println(F("RTC OK; keeping current time."));
    }
  } else {
    Serial.println(F("RTC not found (skip set)."));
  }

  ui.init(lcd);
  ui.setAutoTrack(&autotrack);
  ui.drawCurrentPage(lcd);
}

void loop() {
  ui.handleInput(encoder, fsm, lcd);
  fsm.tick(lcd);
  autotrack.tick(fsm, ui.isUserBusy(), lcd);
  ui.refresh(fsm, lcd);
}
