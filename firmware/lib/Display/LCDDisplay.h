#pragma once

#include <LiquidCrystal_I2C.h>

#include "Display.h"

class LCDDisplay : public Display {
   public:
    LCDDisplay(uint8_t lcd_Addr, uint8_t lcd_cols, uint8_t lcd_rows);
    LCDDisplay(LiquidCrystal_I2C lcd, uint8_t lineWidth, uint8_t nbLines);  // Used for testing only

    void clear();
    void clearLine(u_int8_t line);
    void switchOn();
    void switchOff();

    void displayLine(String text, uint8_t line, DisplayAlignment align);

   private:
    void init();
    LiquidCrystal_I2C* lcd = NULL;
    u_int8_t nbColumns;
    u_int8_t nbLines;
};
