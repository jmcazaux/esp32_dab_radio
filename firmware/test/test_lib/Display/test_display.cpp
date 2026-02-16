
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
