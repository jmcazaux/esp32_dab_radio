#include "LCDDisplay.h"

#include <AdvancedLogger.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>

LCDDisplay::LCDDisplay(uint8_t lcdAddr, uint8_t lcdColumns, uint8_t lcdRows) {
    LOG_DEBUG("Creating LCDDisplay...");
    lcd = new LiquidCrystal_I2C(lcdAddr, lcdColumns, lcdRows);
    nbColumns = lcdColumns;
    nbLines = lcdRows;

    lines = (char**)malloc(sizeof(char*) * nbLines);
    uint8_t i;
    for (i = 0; i < nbColumns; i++) {
        lines[i] = (char*)malloc(sizeof(char) * nbColumns + 1);
    }

    displaySources = new DisplaySource[lcdRows];

    LOG_DEBUG("Initializing LCDDisplay...");
    lcd->init();
    lcd->noAutoscroll();
    switchOff();
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
    LOG_DEBUG("Displaying line \"%s\" at line %d", text, line);

    displaySources[line].setSource(text, align);

    lcd->setCursor(0, line);
    this->padOrTrim(text, lines[line], nbColumns, align);
    LOG_DEBUG("|> %s <|", lines[line]);
    lcd->print(lines[line]);
}

void LCDDisplay::init() {
}

void LCDDisplay::DisplaySource::setSource(char* source, DisplayAlignment align) {
    alignment = align;
    setSource(source);
}

void LCDDisplay::DisplaySource::setSource(char* source) {
    if (_source != NULL) {
        free(_source);
    }

    uint8_t sourceLength = strlen(source);

    _source = new char[sourceLength + 1];
    strcpy(_source, source);
    rollingIndex = 0;
}

char* LCDDisplay::DisplaySource::source() {
    return _source;
}
