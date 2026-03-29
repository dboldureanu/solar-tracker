#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

#include "config.h"
#include "hardware.h"
#include "fsm.h"
#include "ui.h"

// Global objects
hd44780_I2Cexp lcd;
Encoder encoder;
MoveFSM fsm;
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

  if (RTC::isPresent()) {
    RTC::setFromCompileTime();
    Serial.println(F("RTC set from compile time."));
  } else {
    Serial.println(F("RTC not found (skip set)."));
  }

  ui.init(lcd);
  ui.drawCurrentPage(lcd);
}

void loop() {
  ui.handleInput(encoder, fsm, lcd);
  fsm.tick(lcd);
  ui.refresh(fsm, lcd);
}
