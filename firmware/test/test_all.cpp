

#include <gtest/gtest.h>

#if defined(ARDUINO)
#include <Arduino.h>

#include <AdvancedLogger.h>

const char* LOG_FILE_PATH = "/internal/log.txt";
const ulong MAX_LOG_LINES = 500;

void setup() {
    Serial.begin(MONITOR_SPEED);

    AdvancedLogger::setPrintLevel(LogLevel::VERBOSE);
    AdvancedLogger::setSaveLevel(LogLevel::FATAL);
    AdvancedLogger::setMaxLogLines(MAX_LOG_LINES);
    AdvancedLogger::begin(LOG_FILE_PATH);

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
