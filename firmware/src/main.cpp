#include <AdvancedLogger.h>
#include <Arduino.h>
#include <Display.h>
#include <LCDDisplay.h>
#include <LittleFS.h>

#include <string>

#define ST(A) #A
#define STR(A) ST(A)

// Logging related
const char* LOG_FILE_PATH = "/internal/log.txt";
const ulong MAX_LOG_LINES = 500;

Display* display;

void setup() {
    Serial.begin(MONITOR_SPEED);

    // Initialize the logger
    if (!LittleFS.begin(true)) {
        Serial.println("An Error has occurred while mounting LittleFS");
    }

    AdvancedLogger::begin(LOG_FILE_PATH);

    AdvancedLogger::setPrintLevel(LogLevel::VERBOSE);
    AdvancedLogger::setSaveLevel(LogLevel::FATAL);
    AdvancedLogger::setMaxLogLines(MAX_LOG_LINES);
    AdvancedLogger::begin(LOG_FILE_PATH);

    LOG_INFO("Initializing systems...");
    LOG_INFO("- Version %s", STR(VERSION));

    char versionString[30];
    sprintf(versionString, "* Version %s *", STR(VERSION));

    LOG_DEBUG("Initializing display");
    display = new LCDDisplay(0x27, 20, 4);
    display->switchOn();
    display->displayLine(R"(Philips BF501 Redux)", 0, LEFT);
    display->displayLine(R"(Starting systems...)", 1, LEFT);
    display->displayLine(versionString, 2, CENTER);

    delay(3500);
    display->clear();

    LOG_INFO("Systems initialized");
}

void loop() {
    LOG_DEBUG("Starting main loop");
    display->loop();
    delay(250);
}
