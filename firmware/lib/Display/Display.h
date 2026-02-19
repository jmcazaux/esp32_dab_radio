#pragma once

#include <cstdint>

enum DisplayAlignment {
    RIGHT,          // Aligned to the right, might be truncated
    LEFT,           // Aligned to the left, first characters might be removed
    CENTER,         // Text centered, characters might be removed both the the begining and the end
    JUSTIFY,        // Text is justified to stick to both lefat and right edges. Spaces might be added in between words, characters might be removed
    ROLLING_LEFT,   // Text will roll to the left
    ROLLING_RIGHT,  // Text will roll to the right
};

class Display {
   public:
    virtual void clear() {};                  // Clear the display
    virtual void clearLine(uint8_t line) {};  // Clear line `line`

    virtual void switchOn() {};   // Switch the display on
    virtual void switchOff() {};  // Switch the display off

    // To be called in each main loop (used in scrolling, refresh, etc.)
    virtual void tick(unsigned long millis) {};

    // Display `text` on line `line`, with default alignment RIGHT
    void displayLine(char text[], uint8_t line);

    // Display `text` on line `line`, with alignment `align`
    virtual void displayLine(char text[], uint8_t line, DisplayAlignment align) {};

    // Pad (or trim) `source` so that it would fit in `size` characters, respecting `align`.
    // `cycle` is used in case of a rolling alignment and can be an ever increasing int.
    void padOrTrim(char source[], char destination[],uint8_t size, DisplayAlignment align, long cycle);

    // Pad (or trim) `source` so that it would fit in `size` characters, respecting `align`
    void padOrTrim(char source[], char destination[],uint8_t size, DisplayAlignment align) {
        padOrTrim(source, destination, size, align, 0);
    };

   private:
    void trim(char source[], char destination[],uint8_t size, DisplayAlignment align);
    void pad(char source[], char destination[],uint8_t size, DisplayAlignment align);
    void rollLeft(char source[], char destination[],uint8_t size, long cycle);
};
