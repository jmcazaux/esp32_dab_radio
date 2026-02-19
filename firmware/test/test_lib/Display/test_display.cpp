
#include <Arduino.h>
#include <Display.h>
#include <gtest/gtest.h>

Display sut = Display();

TEST(PadOrTrim, PadAlignLEFT) {
    char padded[6];
    sut.padOrTrim("Foo", padded, 5, LEFT);
    EXPECT_STREQ(padded, "Foo  ");
}

TEST(PadOrTrim, PadAlignRIGHT) {
    char padded[6];
    sut.padOrTrim("Foo", padded, 5, RIGHT);
    EXPECT_STREQ(padded, "  Foo");
}

TEST(PadOrTrim, PadAlignCENTER) {
    char padded[7];
    sut.padOrTrim("Foo", padded, 6, CENTER);
    EXPECT_STREQ(padded, " Foo  ");

    sut.padOrTrim("Fooo", padded, 6, CENTER);
    EXPECT_STREQ(padded, " Fooo ");
}

TEST(PadOrTrim, AlignROLLING_LEFTShortString) {
    char padded[7] = "      ";
    sut.padOrTrim("Fooo", padded, 6, ROLLING_LEFT, 0);
    EXPECT_STREQ(padded, "Fooo  ");

    sut.padOrTrim("Fooo", padded, 6, ROLLING_LEFT, 2);
    EXPECT_STREQ(padded, "oo    ");

    sut.padOrTrim("Fooo", padded, 6, ROLLING_LEFT, 3);
    EXPECT_STREQ(padded, "o    F");

    sut.padOrTrim("Fooo", padded, 6, ROLLING_LEFT, 4);
    EXPECT_STREQ(padded, "    Fo");

    sut.padOrTrim("Fooo", padded, 6, ROLLING_LEFT, 5);
    EXPECT_STREQ(padded, "   Foo");

    sut.padOrTrim("Fooo", padded, 6, ROLLING_LEFT, 7);
    EXPECT_STREQ(padded, " Fooo ");

    sut.padOrTrim("Fooo", padded, 6, ROLLING_LEFT, 8);
    EXPECT_STREQ(padded, "Fooo  ");

    sut.padOrTrim("Fooo", padded, 6, ROLLING_LEFT, 9);
    EXPECT_STREQ(padded, "ooo   ");  // Like 1
}

TEST(PadOrTrim, AlignROLLING_LEFTLongString) {
    char padded[7] = "      ";
    sut.padOrTrim("FooBarBaz", padded, 6, ROLLING_LEFT, 0);
    EXPECT_STREQ(padded, "FooBar");

    sut.padOrTrim("FooBarBaz", padded, 6, ROLLING_LEFT, 3);
    EXPECT_STREQ(padded, "BarBaz");

    sut.padOrTrim("FooBarBaz", padded, 6, ROLLING_LEFT, 6);
    EXPECT_STREQ(padded, "Baz   ");

    sut.padOrTrim("FooBarBaz", padded, 6, ROLLING_LEFT, 8);
    EXPECT_STREQ(padded, "z    F");
}

TEST(PadOrTrim, TrimAlignLEFT) {
    char padded[4];
    sut.padOrTrim("FooBar", padded, 3, LEFT);
    EXPECT_STREQ(padded, "Foo");
}

TEST(PadOrTrim, TrimAlignRIGHT) {
    char padded[4];
    sut.padOrTrim("FooBar", padded, 3, RIGHT);
    EXPECT_STREQ(padded, "Bar");
}

TEST(PadOrTrim, TrimAlignCENTER) {
    char padded[4];
    sut.padOrTrim("FooBar", padded, 3, CENTER);
    EXPECT_STREQ(padded, "ooB");
}
