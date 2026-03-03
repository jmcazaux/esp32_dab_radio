#pragma once

#include <LiquidCrystal_I2C.h>

#include "Display.h"

class LCDDisplay : public Display {
   public:
    LCDDisplay(uint8_t lcd_Addr, uint8_t lcd_cols, uint8_t lcd_rows);

    void clear() override;
    void clearLine(u_int8_t line) override;
    void switchOn() override;
    void switchOff() override;
    void tick(unsigned long millis) override;

    void displayLine(const char text[], uint8_t line, DisplayAlignment align) override;

    void displayProgress(uint8_t progress, uint8_t line) override;

   private:
    LiquidCrystal_I2C* lcd = nullptr;
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

        void setSource(const char* source, DisplayAlignment align);
        void setSource(const char* source);
        char* source();

       private:
        char* _source;
    };

    DisplaySource* displaySources;
};
