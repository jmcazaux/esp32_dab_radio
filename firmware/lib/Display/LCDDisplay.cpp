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

void LCDDisplay::displayLine(char text[], uint8_t line, DisplayAlignment align) {
    LOG_INFO("Displaying line \"%s\" at line %d", text, line);
    lcd->setCursor(0, line);
    char charLine[nbColumns + 1];
    this->padOrTrim(text, charLine, nbColumns, align);
    LOG_DEBUG(". -> |> %s <|", charLine);
    lcd->print(charLine);
}

void LCDDisplay::init() {
    LOG_DEBUG("Initializing LCDDisplay...");
    lcd->init();
    lcd->noAutoscroll();
    switchOff();
}
