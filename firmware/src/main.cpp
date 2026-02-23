#include <AdvancedLogger.h>
#include <Arduino.h>
#include <AudioSource.h>
#include <Bluetooth.h>
#include <DABRadio.h>
#include <DABShield.h>
#include <Display.h>
#include <FMRadio.h>
#include <LCDDisplay.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <RotaryEncoder.h>

#include <string>

#define ST(A) #A
#define STR(A) ST(A)

// Pins
#define SELECTOR_ENCODER_SW 5    // to SW pin of the mode selector rotary encoder
#define SELECTOR_ENCODER_DT 16   // to DT pin of the mode selector rotary encoder
#define SELECTOR_ENCODER_CLK 17  // to CLK pin of the mode selector rotary encoder

// Logging related
constexpr char* LOG_FILE_PATH = "/internal/log.txt";
constexpr ulong MAX_LOG_LINES = 500;

// Delays & timings
constexpr ulong SWITCH_SOURCE_DELAY = 400;      // Delay between a source is selected and the source become active (avoid switching source at each encoder tick)
constexpr ulong SELECT_SOURCE_MIN_DELAY = 200;  // Delay between 2 changes of the selected source (avoid two many changes when rotating the knob fast)

// Preferences
constexpr char GENERAL_PREF_NAMESPACE[] = "general";
constexpr char PREVIOUS_SOURCE_KEY[] = "previousSource";
Preferences preferences;

// Sources
constexpr int nbSources = 3;
AudioSource* sources[nbSources];

uint8_t currentSource = 0;
int selectedSource = currentSource;
long selectedSourceTime = 0;

// Devices
DAB dab;
Display* display;
RotaryEncoder selectorEncoder(SELECTOR_ENCODER_DT, SELECTOR_ENCODER_CLK, RotaryEncoder::LatchMode::TWO03);

void setup() {
    Serial.begin(MONITOR_SPEED);
    preferences.begin(GENERAL_PREF_NAMESPACE, false);

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

    char versionString[30];
    sprintf(versionString, "* Version %s *", STR(VERSION));

    LOG_DEBUG("Initializing display...");
    display = new LCDDisplay(0x27, 20, 4);
    display->switchOn();
    display->displayLine("Philips BF501 Redux", 0, LEFT);
    display->displayLine("Starting systems...", 1, LEFT);
    display->displayLine(versionString, 2, CENTER);

    LOG_DEBUG("Initilizing audio sources...");
    sources[0] = new FMRadio(display, &dab);
    sources[1] = new DABRadio(display, &dab);
    sources[2] = new Bluetooth(display);

    LOG_INFO("Initialized Audio sources: ");
    for (u_int8_t i = 0; i < nbSources; i++) {
        LOG_INFO(" -> #%d: %s", i, sources[i]->name);
    }

    delay(800);

    // Testing display
    display->clearLine(1);
    display->displayLine("France Inter", 2, CENTER);
    display->displayLine("La plus grande matinale de France avec Florence Paracuellos", 3, ROLLING_LEFT);

    // Restoring previous souce
    currentSource = preferences.getInt(PREVIOUS_SOURCE_KEY, 0) % nbSources;  // Just to make sure
    selectedSource = currentSource;
    sources[currentSource]->activate();

    LOG_INFO("Systems initialized");
}

void loop() {
    static int selectorPosition = 0;

    // Tick everything that needs ticking
    display->tick(millis());
    selectorEncoder.tick();

    int newPos = selectorEncoder.getPosition();
    if (selectorPosition != newPos) {
        selectorPosition = newPos;
        if (selectedSourceTime + SELECT_SOURCE_MIN_DELAY < millis()) {
            int step = selectorEncoder.getDirection() == RotaryEncoder::Direction::CLOCKWISE ? 1 : -1;
            selectedSource = (selectedSource + step) % nbSources;  // Keep in [0..nbSources]
            // When turning anti-clockwise, we want to go from the first to the last, then one before the last etc.
            selectedSource = (selectedSource < 0 ? selectedSource + nbSources : selectedSource);
            selectedSourceTime = millis();

            LOG_INFO("Selecting source #%d - %s", selectedSource, sources[selectedSource]->name);

            display->displayLine(sources[selectedSource]->name, 0);
        }
    }

    if (selectedSource != currentSource && (selectedSourceTime + SWITCH_SOURCE_DELAY) < millis()) {
        // Deactivating previous source
        sources[currentSource]->deactivate();

        // Activating new source
        currentSource = selectedSource;
        display->displayLine(sources[currentSource]->name, 0);
        sources[currentSource]->activate();

        preferences.putInt(PREVIOUS_SOURCE_KEY, currentSource);

        selectedSourceTime = 0;
    }

    delay(10);
}
