
#include "Display.h"

#include <AdvancedLogger.h>
#include <Arduino.h>
#include <string.h>

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

void Display::padOrTrim(char source[], char destination[], uint8_t size, DisplayAlignment align, uint8_t from) {
    if (strlen(source) > size) {
        return trim(source, destination, size, align, from);
    } else {
        return pad(source, destination, size, align, from);
    }
};

void Display::pad(char source[], char destination[], uint8_t size, DisplayAlignment align, uint8_t from) {
    uint8_t sourceLength = strlen(source);
    uint8_t toLength = size;

    switch (align) {
        case RIGHT: {
            uint8_t prefixLength = toLength - sourceLength;
            prefixAndSuffixStringWithSpaces(source, destination, prefixLength, 0);
            break;
        }

        case LEFT: {
            uint8_t suffixLength = toLength - sourceLength;
            prefixAndSuffixStringWithSpaces(source, destination, 0, suffixLength);
            break;
        }

        case CENTER: {
            uint8_t prefixLength = (toLength - sourceLength) / 2;
            uint8_t suffixLength = toLength - sourceLength - prefixLength;
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

void Display::trim(char source[], char destination[], uint8_t size, DisplayAlignment align, uint8_t from) {
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
