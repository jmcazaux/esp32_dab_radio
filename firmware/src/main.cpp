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
#include <OneButton.h>
#include <Preferences.h>
#include <RotaryEncoder.h>
#include <SPI.h>

#include <string>

// Enable power management
#define CONFIG_PM_ENABLE 1

#define ST(A) #A
#define STR(A) ST(A)

// Pins
#define SELECTOR_ENCODER_SW 5    // to SW pin of the mode selector rotary encoder
#define SELECTOR_ENCODER_DT 16   // to DT pin of the mode selector rotary encoder
#define SELECTOR_ENCODER_CLK 17  // to CLK pin of the mode selector rotary encoder
#define TUNE_ENCODER_SW 13       // to SW pin of the mode selector rotary encoder
#define TUNE_ENCODER_DT 35       // to DT pin of the mode selector rotary encoder
#define TUNE_ENCODER_CLK 34      // to CLK pin of the mode selector rotary encoder

#define DAB_SPI_SLAVE_SELECT 12

// Logging related
constexpr char LOG_FILE_PATH[] = "/internal/log.txt";
constexpr ulong MAX_LOG_LINES = 500;

// Delays & timings
constexpr ulong SWITCH_SOURCE_DELAY = 400;      // Delay between a source is selected and the source become active (avoid switching source at each encoder tick)
constexpr ulong SELECT_SOURCE_MIN_DELAY = 200;  // Delay between 2 changes of the selected source (avoid two many changes when rotating the knob fast)

// Preferences
constexpr char GENERAL_PREF_NAMESPACE[] = "general";
constexpr char PREVIOUS_SOURCE_KEY[] = "previousSource";
Preferences preferences;

// Sources
constexpr int HIGH_CPU_CLOCK_MHZ = 240;
constexpr int LOW_CPU_CLOCK_MHZ = 40;
constexpr int nbSources = 3;
AudioSource* sources[nbSources];

uint8_t currentSourceIndex = 0;
int selectedSourceIndex = currentSourceIndex;
long lastSelectedSourceTime = 0;

// Devices
DAB dab;
Display* display;
RotaryEncoder selectorEncoder(SELECTOR_ENCODER_DT, SELECTOR_ENCODER_CLK, RotaryEncoder::LatchMode::TWO03);
RotaryEncoder tuneEncoder(TUNE_ENCODER_DT, TUNE_ENCODER_CLK, RotaryEncoder::LatchMode::TWO03);
OneButton selectorButton = OneButton(SELECTOR_ENCODER_SW, true, true);
OneButton tuneButton = OneButton(TUNE_ENCODER_SW, true, true);
constexpr uint BUTTON_DOUBLECLICK_DELAY_MS = 300;

void logFrequencies() {
    LOG_DEBUG("Frequencies:");
    LOG_DEBUG(" > CPU clock:      %dMHz", getCpuFrequencyMhz());
    LOG_DEBUG(" > ABP frequency:  %dMHz", getApbFrequency() / 1000000);
    LOG_DEBUG(" > XTAL frequency: %dMHz", getXtalFrequencyMhz());
}

void serviceData() {
    LOG_DEBUG("Got service data...", currentSourceIndex);
    sources[currentSourceIndex]->refreshInformation();
}

void DABSpiMsg(unsigned char* data, uint32_t len) {
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));  // 2MHz for starters...
    digitalWrite(DAB_SPI_SLAVE_SELECT, LOW);
    SPI.transfer(data, len);
    digitalWrite(DAB_SPI_SLAVE_SELECT, HIGH);
    SPI.endTransaction();
}

void enableRadio() {
    LOG_DEBUG("Switching radio ON...");
    dab.setCallback(serviceData);
    dab.mute(true, true);  // Avoid "tuning" noises
    dab.begin(1);  // Actual mode set by the AudioSource
    if (dab.error != 0) {
        LOG_ERROR("DABShield error: %s", dab.error);
    }
    LOG_INFO("Switched radio ON");
}

void disableRadio() {
    LOG_DEBUG("Switching radio OFF...");
    dab.end();
    LOG_INFO("Switched radio OFF");
}

void switchSource(int fromSourceIdx, int toSourceIdx) {
    AudioSource* toSource = sources[toSourceIdx];
    AudioSource* fromSource = nullptr;
    if (fromSourceIdx >= 0) {
        fromSource = sources[fromSourceIdx];
    }

    if (fromSource != nullptr) {
        fromSource->deactivate();
    }

    // Tuning CPU
    if (fromSource == nullptr || toSource->needsLowCpuFrequency != fromSource->needsLowCpuFrequency) {
        const long frequency = toSource->needsLowCpuFrequency ? LOW_CPU_CLOCK_MHZ : HIGH_CPU_CLOCK_MHZ;
        LOG_DEBUG("Setting frequency to %ldMhz...", frequency);
        Serial.flush();  // Console is mingled at lowest frequencies. Need to flush and refresh buadRate
        setCpuFrequencyMhz(frequency);
        Serial.updateBaudRate(MONITOR_SPEED);
        logFrequencies();
        LOG_INFO("Set frequency to %ldMhz", frequency);
    }

    // Toggle DAB: doing here to avoid on/off/on when switching from FM to DAB
    if (fromSource == nullptr || toSource->needsRadio != fromSource->needsRadio) {
        if (toSource->needsRadio) {
            enableRadio();
        } else {
            disableRadio();
        }
    }

    // TODO:Bluetooth

    toSource->activate();

    currentSourceIndex = toSourceIdx;
    selectedSourceIndex = toSourceIdx;
}

void selectorClicked() {
    LOG_DEBUG("Selector clicked");
}

void selectorDoubleClicked() {
    LOG_DEBUG("Selector double-clicked");
}

void selectorPressStart() {
    LOG_DEBUG("Selector long-press started");
}

void selectorPressStopped() {
    LOG_DEBUG("Selector long-press stopped");
}

void tuneClicked() {
    LOG_DEBUG("Tune clicked");
    sources[currentSourceIndex]->tunePressed();
}

void tuneDoubleClicked() {
    LOG_DEBUG("Tune double-clicked");
    sources[currentSourceIndex]->tuneDoublePressed();
}

void tunePressStart() {
    LOG_DEBUG("Tune long-press started");
    sources[currentSourceIndex]->tuneLongPressed();
}

void tunePressStopped() {
    LOG_DEBUG("Tune long-press stopped");
}

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

    LOG_DEBUG("Initializing audio sources...");
    sources[0] = new FMRadio(display, &dab);
    sources[1] = new DABRadio(display, &dab);
    sources[2] = new Bluetooth(display);

    pinMode(DAB_SPI_SLAVE_SELECT, OUTPUT);
    digitalWrite(DAB_SPI_SLAVE_SELECT, HIGH);
    SPI.begin();
    dab.speaker(SPEAKER_DIFF);

    LOG_INFO("Initialized Audio sources: ");
    for (u_int8_t i = 0; i < nbSources; i++) {
        LOG_INFO(" -> #%d: %s", i, sources[i]->name);
    }

    LOG_DEBUG("Initializing buttons...");
    selectorButton.setClickMs(BUTTON_DOUBLECLICK_DELAY_MS);
    selectorButton.attachClick(selectorClicked);
    selectorButton.attachDoubleClick(selectorDoubleClicked);
    selectorButton.attachLongPressStart(selectorPressStart);
    selectorButton.attachLongPressStop(selectorPressStopped);

    tuneButton.setClickMs(BUTTON_DOUBLECLICK_DELAY_MS);
    tuneButton.attachClick(tuneClicked);
    tuneButton.attachDoubleClick(tuneDoubleClicked);
    tuneButton.attachLongPressStart(tunePressStart);
    tuneButton.attachLongPressStop(tunePressStopped);
    LOG_INFO("Initialized buttons");

    // Restoring previous source
    currentSourceIndex = preferences.getInt(PREVIOUS_SOURCE_KEY, 0) % nbSources;  // Just to make sure
    display->clear();
    switchSource(-1, currentSourceIndex);


    LOG_INFO("Systems initialized");
}

void loop() {
    static int selectorPosition = 0;
    static int tunerPosition = 0;

    // Tick everything that needs ticking
    display->tick(millis());
    selectorEncoder.tick();
    tuneEncoder.tick();
    selectorButton.tick();
    tuneButton.tick();

    if (sources[currentSourceIndex]->needsRadio) {
        dab.task();
    }

    int newSelectorPosition = selectorEncoder.getPosition();
    if (selectorPosition != newSelectorPosition) {
        selectorPosition = newSelectorPosition;
        if (lastSelectedSourceTime + SELECT_SOURCE_MIN_DELAY < millis()) {
            int step = selectorEncoder.getDirection() == RotaryEncoder::Direction::CLOCKWISE ? 1 : -1;
            selectedSourceIndex = (selectedSourceIndex + step) % nbSources;  // Keep in [0..nbSources]
            // When turning anti-clockwise, we want to go from the first to the last, then one before the last etc.
            selectedSourceIndex = (selectedSourceIndex < 0 ? selectedSourceIndex + nbSources : selectedSourceIndex);
            lastSelectedSourceTime = millis();

            LOG_INFO("Selecting source #%d - %s", selectedSourceIndex, sources[selectedSourceIndex]->name);

            display->displayLine(sources[selectedSourceIndex]->name, 0);
        }
    }

    if (selectedSourceIndex != currentSourceIndex && (lastSelectedSourceTime + SWITCH_SOURCE_DELAY) < millis()) {
        // Deactivating previous source
        switchSource(currentSourceIndex, selectedSourceIndex);
        preferences.putInt(PREVIOUS_SOURCE_KEY, currentSourceIndex);

        lastSelectedSourceTime = 0;
    }

    int newTunerPosition = tuneEncoder.getPosition();
    if (tunerPosition != newTunerPosition) {
        RotaryEncoder::Direction tuneDirection = tuneEncoder.getDirection();
        LOG_DEBUG("Tuning %s", tuneDirection == RotaryEncoder::Direction::CLOCKWISE ? "UP" : "DOWN");
        if (tuneDirection == RotaryEncoder::Direction::CLOCKWISE) {
            sources[currentSourceIndex]->tuneUp();
        } else {
            sources[currentSourceIndex]->tuneDown();
        }
        tunerPosition = newTunerPosition;
    }

    delay(10);
}
