#include <Arduino.h>

#include <AdvancedLogger.h>
#include <LittleFS.h>

#define ST(A) #A
#define STR(A) ST(A)

#define STR(VERSION) ST(VERSION)

// Logging related
const char* LOG_FILE_PATH = "/internal/log.txt";
const ulong MAX_LOG_LINES = 500;


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
  LOG_INFO("* Version %s", STR(VERSION));


  LOG_INFO("Systems initialized");

}

void loop() {
  LOG_DEBUG("Starting main loop");
  delay(250);
}

