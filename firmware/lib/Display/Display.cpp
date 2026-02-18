
#include "Display.h"

#include <AdvancedLogger.h>
#include <Arduino.h>
#include <string.h>

#include <stdexcept>

void prefixAndSuffixStringWithSpaces(char source[], char destination[], uint8_t prefixLength, uint8_t suffixLength) {
    uint8_t i;

    for (i = 0; i < prefixLength; i++) {
        destination[i] = ' ';
    }

    for (i = 0; i < strlen(source); i++) {
        destination[prefixLength + i] = source[i];
    }

    for (i = 0; i < suffixLength; i++) {
        destination[prefixLength + strlen(source) + i] = ' ';
    }
}

void copySubstring(char source[], char destination[], uint8_t from, uint8_t length) {
    uint8_t i;

    for (i = 0; i < length && i + from < strlen(source); i++) {
        destination[i] = source[from + i];
    }
}

void Display::displayLine(char text[], uint8_t line) {
    displayLine(text, line, DisplayAlignment::LEFT);
};

void Display::padOrTrim(char source[], char destination[], int size, DisplayAlignment align, long cycle) {
    if ((align == ROLLING_LEFT || align == ROLLING_RIGHT) && cycle < 0) {
        throw std::invalid_argument("'cycle' must be provided for rolling alignments");
    }

    if (align == ROLLING_LEFT) {
        rollLeft(source, destination, size, cycle);
    } else if (strlen(source) > size) {
        return trim(source, destination, size, align);
    } else {
        return pad(source, destination, size, align);
    }
};

void Display::pad(char source[], char destination[], int size, DisplayAlignment align) {
    uint8_t sourceLength = strlen(source);

    switch (align) {
        case RIGHT: {
            uint8_t prefixLength = size - sourceLength;
            prefixAndSuffixStringWithSpaces(source, destination, prefixLength, 0);
            break;
        }

        case LEFT: {
            uint8_t suffixLength = size - sourceLength;
            prefixAndSuffixStringWithSpaces(source, destination, 0, suffixLength);
            break;
        }

        case CENTER: {
            uint8_t prefixLength = (size - sourceLength) / 2;
            uint8_t suffixLength = size - sourceLength - prefixLength;
            prefixAndSuffixStringWithSpaces(source, destination, prefixLength, suffixLength);
            break;
        }

        default: {
            LOG_ERROR("Alignment not implemented yet (ordinal %d)", align);
            strcpy(destination, "Not implmented");
            break;
        }
    }

    destination[size] = '\0';
}

void Display::rollLeft(char source[], char destination[], int size, long cycle) {
    int sourceLength = strlen(source);
    int apparentLength = sourceLength + size - 2;  // Length of string that would roll if it was one (we want the first character to reappear when the last one is displayed)
    int sourceFrom = cycle % apparentLength;       // `cycle` will be an ever increasing counter, so apply modulus
    int i;

    if (sourceFrom < sourceLength) {
        // "Foobar" -> "bar     " || "r      F"

        // "bar"
        copySubstring(source, destination, sourceFrom, min(sourceLength - sourceFrom, int(size)));

        // "bar      "
        for (i = sourceLength - sourceFrom; i < size; i++) {
            destination[i] = ' ';
        }

        // "r      F"
        if (sourceFrom == sourceLength - 1) {
            destination[size - 1] = source[0];
        }

    } else {
        // "    Foob" || " Foobar "
        // "        "
        for (i = 0; i < size; i++) {
            destination[i] = ' ';
        }

        // "    Foob"
        for (i = 0; i < sourceLength && (apparentLength - sourceFrom + i) < size; i++) {
            destination[apparentLength - sourceFrom + i] = source[i];
        }
    }

    destination[size] = '\0';
}

void Display::trim(char source[], char destination[], int size, DisplayAlignment align) {
    uint8_t sourceLength = strlen(source);
    switch (align) {
        case RIGHT: {
            copySubstring(source, destination, sourceLength - size, size);
            break;
        }

        case LEFT: {
            strncpy(destination, source, size);
            break;
        }

        case CENTER: {
            uint8_t copyFrom = (sourceLength - size) / 2;
            copySubstring(source, destination, copyFrom, size);
            ;
            break;
        }

        default: {
            LOG_ERROR("Alignment not implemented yet (ordinal %d)", align);
            strcpy(destination, "Not implmented");
            break;
        }
    }

    destination[size] = '\0';
}
