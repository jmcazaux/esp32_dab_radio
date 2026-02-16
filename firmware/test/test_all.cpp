

#include <gtest/gtest.h>

#if defined(ARDUINO)
#include <Arduino.h>

void setup() {
    Serial.begin(MONITOR_SPEED);

    ::testing::InitGoogleTest();
}

void loop() {
    // Run tests
    if (RUN_ALL_TESTS())
        ;

    // sleep 1 sec
    delay(1000);
}

#else
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (RUN_ALL_TESTS())
        ;
    // Always return zero-code and allow PlatformIO to parse results
    return 0;
}
#endif
