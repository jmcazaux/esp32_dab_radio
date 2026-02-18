#pragma once

#include <LiquidCrystal_I2C.h>

#include "Display.h"

class LCDDisplay : public Display {
   public:
    LCDDisplay(uint8_t lcd_Addr, uint8_t lcd_cols, uint8_t lcd_rows);

    void clear();
    void clearLine(u_int8_t line);
    void switchOn();
    void switchOff();
    void tick(unsigned long millis);

    void displayLine(char text[], uint8_t line, DisplayAlignment align);

   private:
    void init();
    LiquidCrystal_I2C* lcd = NULL;
    u_int8_t nbColumns;
    u_int8_t nbLines;

    unsigned long lastCycleTime = 0;

    char** lines;

    // Keep a copy of what is currently displayed (rolling index, etc)
    class DisplaySource {
       public:
        DisplayAlignment alignment;
        uint8_t rollingIndex;

        DisplaySource() : alignment{LEFT},
                          rollingIndex{0},
                          _source{nullptr} {
        }

        void setSource(char* source, DisplayAlignment align);
        void setSource(char* source);
        char* source();

       private:
        char* _source;
    };

    DisplaySource* displaySources;
};
