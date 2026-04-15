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
  delay(500);

  initRelays();
  encoder.init();
  pinMode(INA_ALERT_PIN, INPUT_PULLUP);

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setTimeOut(50);

  int lcdStatus = lcd.begin(LCD_COLS, LCD_ROWS);
  if (lcdStatus) {
    Serial.print(F("LCD init error: ")); Serial.println(-lcdStatus);
  }
  lcd.backlight();

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
