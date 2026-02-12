#include "LCDDisplay.h"

#include <AdvancedLogger.h>
#include <LiquidCrystal_I2C.h>

LCDDisplay::LCDDisplay(uint8_t lcdAddr, uint8_t lcdColumns, uint8_t lcdRows) {
    LOG_DEBUG("Creating LCDDisplay...");
    lcd = new LiquidCrystal_I2C(lcdAddr, lcdColumns, lcdRows);
    nbColumns = lcdColumns;
    nbLines = lcdRows;
    init();
}

LCDDisplay::LCDDisplay(LiquidCrystal_I2C lcd, uint8_t nbColumns, uint8_t nbLines) {
    this->lcd = &lcd;
    this->nbColumns = nbColumns;
    this->nbLines = nbLines;
    init();
}

void LCDDisplay::switchOn() {
    lcd->on();
    lcd->clear();
    lcd->backlight();
}

void LCDDisplay::switchOff() {
    lcd->noBacklight();
    lcd->clear();
    lcd->off();
}

void LCDDisplay::clear() {
    lcd->clear();
}

void LCDDisplay::clearLine(u_int8_t line) {
  if (line >= nbLines) {
      LOG_ERROR("Cannot clear line %d (only %d lines)... Ignoring.", line, nbLines);
      return;
  }

  lcd->setCursor(0, line);
  for (uint8_t i = 0; i < nbColumns; i++) {
      lcd->print(" ");
  }
}

void LCDDisplay::displayLine(String text, uint8_t line, DisplayAlignment align) {
    lcd->setCursor(0, line);
    lcd->print(text);
}

void LCDDisplay::init() {
    LOG_DEBUG("Initializing LCDDisplay...");
    lcd->init();
    lcd->noAutoscroll();
    switchOff();
}
