#include <AdvancedLogger.h>
#include <Arduino.h>
#include <AudioSource.h>
#include <Display.h>
#include <LCDDisplay.h>
#include <LittleFS.h>
#include <RotaryEncoder.h>

#include <string>

#define ST(A) #A
#define STR(A) ST(A)

// Pins
#define SELECTOR_ENCODER_SW 5    // to SW pin of the mode selector rotary encoder
#define SELECTOR_ENCODER_DT 16   // to DT pin of the mode selector rotary encoder
#define SELECTOR_ENCODER_CLK 17  // to CLK pin of the mode selector rotary encoder

// Logging related
const char* LOG_FILE_PATH = "/internal/log.txt";
const ulong MAX_LOG_LINES = 500;

// Delays & timings
const ulong SWITCH_SOURCE_DELAY = 400;  // Delay between a source is selected and the source become active (avoid switching source at each encoder tick)

Display* display;

static const char RADIO_FM[] = "Radio FM";
static const char RADIO_DAB[] = "Radio DAB";
static const char BLUETOOTH[] = "Bluetooth";

AudioSource fm = AudioSource(RADIO_FM);
AudioSource dab = AudioSource(RADIO_DAB);
AudioSource bluetooth = AudioSource(BLUETOOTH);

AudioSource sources[] = {fm, dab, bluetooth};
uint8_t nbSources = std::size(sources);

uint8_t currentSource = 0;
int selectedSource = -1;
long selectedSourceTime = millis();

RotaryEncoder selectorEncoder(SELECTOR_ENCODER_DT, SELECTOR_ENCODER_CLK, RotaryEncoder::LatchMode::TWO03);

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

    LOG_INFO("Initializing systems...");
    LOG_INFO("*** Version %s ***", STR(VERSION));

    LOG_INFO("Initializing Audio sources: ");
    for (u_int8_t i = 0; i < nbSources; i++) {
        LOG_INFO(" -> #%d: %s", i + 1, sources[i].name);
    }

    char versionString[30];
    sprintf(versionString, "* Version %s *", STR(VERSION));

    LOG_DEBUG("Initializing display...");
    display = new LCDDisplay(0x27, 20, 4);
    display->switchOn();
    display->displayLine("Philips BF501 Redux", 0, LEFT);
    display->displayLine("Starting systems...", 1, LEFT);
    display->displayLine(versionString, 2, CENTER);
    delay(2000);

    // Testing display
    display->clearLine(1);
    display->clearLine(2);
    display->displayLine("France Inter", 2, CENTER);
    display->displayLine("La plus grande matinale de France avec Florence Paracuellos", 3, ROLLING_LEFT);

    LOG_INFO("Systems initialized");
    display->displayLine(sources[currentSource].name, 0);
}

void loop() {
    static int selectorPosition = 0;

    // Tick everything that needs ticking
    display->tick(millis());
    selectorEncoder.tick();


    int newPos = selectorEncoder.getPosition();
    if (selectorPosition != newPos) {
        selectedSource = (newPos / 2) % nbSources;
        // When turning anti-clockwise, we want to go from the first to the last, then one before the last etc.
        selectedSource = (selectedSource < 0 ? selectedSource + nbSources : selectedSource);
        selectedSourceTime = millis();
        selectorPosition = newPos;

        LOG_INFO("Selecting source #%d (pos %d)", selectedSource, selectorPosition);

        display->displayLine(sources[selectedSource].name, 0);
    }

    if (selectedSource >= 0 && (selectedSourceTime + SWITCH_SOURCE_DELAY) < millis()) {
        currentSource = selectedSource;
        selectedSource = -1;
        selectedSourceTime = 0;

        display->displayLine(sources[currentSource].name, 0);
        LOG_INFO("Switched to source %d", currentSource);
    }


    delay(10);
}
