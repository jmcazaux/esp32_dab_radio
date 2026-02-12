
#include "Display.h"

void Display::displayLine(String text, uint8_t line) {
    displayLine(text, line, DisplayAlignment::RIGHT);
};

String Display::padOrTrim(String source, uint8_t width, DisplayAlignment align, uint8_t from) {
    return "Not implemented yet";
};
