#include "LCDDisplay.h"

#include <AdvancedLogger.h>
#include <LiquidCrystal_I2C.h>
#include <cstring>

constexpr long ROLLING_TEXT_UPDATE_INTERVAL_MS = 600;

byte BLOCK[8] = {
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111
};

LCDDisplay::LCDDisplay(uint8_t lcdAddr, uint8_t lcdColumns, uint8_t lcdRows) {
    LOG_DEBUG("Creating LCDDisplay...");
    lcd = new LiquidCrystal_I2C(lcdAddr, lcdColumns, lcdRows);

    nbColumns = lcdColumns;
    nbLines = lcdRows;

    lines = new char*[nbLines];
    for(uint8_t i = 0; i < nbLines; i++) {
        lines[i] = new char[nbColumns + 1];
    }

    displaySources = new DisplaySource[lcdRows];
    LOG_DEBUG("Initializing LCDDisplay...");
    lcd->init();
    lcd->noAutoscroll();
    // create a new character
    lcd->createChar(0, BLOCK);

    this->switchOff();
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
    for (int i = 0; i < nbLines; i++) {
        displaySources[i].setSource("");
    }
    lcd->clear();
}

void LCDDisplay::clearLine(u_int8_t line) {
    if (line >= nbLines) {
        LOG_WARNING("Cannot clear line %d (only %d lines)... Ignoring.", line, nbLines);
        return;
    }

    displayLine(" ", line, LEFT);
}

void LCDDisplay::displayLine(const char text[], uint8_t line, DisplayAlignment align) {
    if (line >= nbLines) {
        LOG_WARNING("Cannot display line %d (only %d lines)... Ignoring.", line, nbLines);
        return;
    }

    if (displaySources[line].source() != nullptr
        && strcmp(displaySources[line].source(), text) == 0
        && displaySources[line].alignment == align) {
        // Same text and alignment, no need to change anything
        return;
    }

    displaySources[line].setSource(text, align);

    this->padOrTrim(text, lines[line], nbColumns, align);
    lcd->setCursor(0, line);
    lcd->print(lines[line]);
}

void LCDDisplay::displayProgress(const uint8_t progress, const uint8_t line) {
    const uint8_t actualProgress = min(100,max(0, static_cast<int>(progress)));
    const auto nbBlocks = static_cast<uint8_t>(round(actualProgress / (100.0 / nbColumns)));
    for (uint8_t i = 0; i < nbBlocks; i++) {
        lcd->setCursor(i, line);
        lcd->write(0);
    }
}

void LCDDisplay::tick(unsigned long millis) {
    if (millis < lastCycleTime + ROLLING_TEXT_UPDATE_INTERVAL_MS) {
        return;
    }

    lastCycleTime = millis;

    for(uint8_t i = 0; i < nbLines; i++) {
        DisplaySource* source = &displaySources[i];
        if (source->alignment == ROLLING_LEFT) {
            source->rollingIndex++;
            this->padOrTrim(source->source(), lines[i], nbColumns, source->alignment, source->rollingIndex);
            lcd->setCursor(0, i);
            lcd->print(lines[i]);
        }
    }
}

void LCDDisplay::DisplaySource::setSource(const char* source, DisplayAlignment align) {
    alignment = align;
    setSource(source);
}

void LCDDisplay::DisplaySource::setSource(const char* source) {
    if (_source != nullptr) {
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
