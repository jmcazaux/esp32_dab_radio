#pragma once

#include <WString.h>

#include <cstdint>

enum DisplayAlignment {
    RIGHT,         // Aligned to the right, might be truncated
    LEFT,          // Aligned to the left, first characters might be removed
    CENTER,        // Text centered, characters might be removed both the the begining and the end
    JUSTIFY,       // Text is justified to stick to both lefat and right edges. Spaces might be added in between words, characters might be removed
    ROLLING_LEFT,  // Text will roll to the left
    ROLLING_RIGHT, // Text will roll to the right
};

class Display {
   public:
    virtual void clear() {};                  // Clear the display
    virtual void clearLine(uint8_t line) {};  // Clear line `line`

    virtual void switchOn() {};   // Switch the display on
    virtual void switchOff() {};  // Switch the display off

    // To be called in each main loop (used in scrolling, refresh, etc.)
    virtual void loop() {};

    // Display `text` on line `line`, with default alignment RIGHT
    void displayLine(String text, uint8_t line);

    // Display `text` on line `line`, with alignment `align`
    virtual void displayLine(String text, uint8_t line, DisplayAlignment align) {};

    // Pad (or trim) `source` so that it would fit in `toWidth` characters, respecting `align` and starting at position `from`
    String padOrTrim(String source, uint8_t toWidth, DisplayAlignment align, uint8_t from);

    // Pad (or trim) `source` so that it would fit in `toWidth` characters, respecting `align` and starting from the begining of the string
    String padOrTrim(String source, uint8_t toWidth, DisplayAlignment align) {
        return padOrTrim(source, toWidth, align, 0);
    };
};
