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
#include <MainStrings.h>
#include <OneButton.h>
#include <Preferences.h>
#include <RotaryEncoder.h>
#include <SPI.h>
#include <AudioTools.h>
#include <BluetoothA2DPSink.h>
#include <esp_log.h>

#include "BluetoothA2DPSink.h"


namespace dabradio = com::ironbird::esp32dabradio;

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
#define I2S_BCK 33               // Audio data bit clock (from I2S master = DABShield)
#define I2S_SDOUT 32             // Audio data output (to DAC)
#define I2S_WS 15                // Audio data left and right clock (from I2S master = DABShield)

#define DAB_SPI_SLAVE_SELECT 12

// Logging related
constexpr char LOG_FILE_PATH[] = "/internal/log.txt";
constexpr ulong MAX_LOG_LINES = 500;

constexpr char LOG_TAG[] = "E32DR";

// Delays & timings
constexpr ulong SWITCH_SOURCE_DELAY = 400;
// Delay between a source is selected and the source become active (avoid switching source at each encoder tick)
constexpr ulong SELECT_SOURCE_MIN_DELAY = 200;
// Delay between 2 changes of the selected source (avoid two many changes when rotating the knob fast)

// Preferences
constexpr char GENERAL_PREF_NAMESPACE[] = "general";
constexpr char PREVIOUS_SOURCE_KEY[] = "previousSource";
Preferences preferences;

// Sources
constexpr int HIGH_CPU_CLOCK_MHZ = 240;
constexpr int LOW_CPU_CLOCK_MHZ = 40;
constexpr int NB_SOURCES = 3;
dabradio::AudioSource *sources[NB_SOURCES];

uint8_t currentSourceIndex = 0;
int selectedSourceIndex = currentSourceIndex;
dabradio::AudioSource *currentSource = nullptr;
unsigned long lastSelectedSourceTime = 0;


// Devices
DAB dab;

dabradio::Display *display;

I2SStream i2s;
BluetoothA2DPSink bluetoothSink(i2s);

RotaryEncoder selectorEncoder(SELECTOR_ENCODER_DT, SELECTOR_ENCODER_CLK, RotaryEncoder::LatchMode::TWO03);
RotaryEncoder tuneEncoder(TUNE_ENCODER_DT, TUNE_ENCODER_CLK, RotaryEncoder::LatchMode::TWO03);

OneButton selectorButton(SELECTOR_ENCODER_SW, true, true);
OneButton tuneButton(TUNE_ENCODER_SW, true, true);
constexpr uint BUTTON_DOUBLECLICK_DELAY_MS = 300;

void logCpuFrequencies() {
    LOG_DEBUG("Frequencies:");
    LOG_DEBUG(" > CPU clock:      %dMHz", getCpuFrequencyMhz());
    LOG_DEBUG(" > ABP frequency:  %dMHz", getApbFrequency() / 1000000);
    LOG_DEBUG(" > XTAL frequency: %dMHz", getXtalFrequencyMhz());
}

void dabServiceDataCallback() {
    LOG_DEBUG("Got DAB service data...", currentSourceIndex);
    currentSource->displayInformation();
}


void DABSpiMsg(unsigned char *data, uint32_t len) {
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0)); // 2MHz for starters...
    digitalWrite(DAB_SPI_SLAVE_SELECT, LOW);
    SPI.transfer(data, len);
    digitalWrite(DAB_SPI_SLAVE_SELECT, HIGH);
    SPI.endTransaction();
}

void enableBluetooth() {
    LOG_DEBUG("Enabling I2S...");
    auto cfg = i2s.defaultConfig();
    cfg.pin_bck = I2S_BCK;
    cfg.pin_ws = I2S_WS;
    cfg.pin_data = I2S_SDOUT;

    auto i2sInitialized = i2s.begin(cfg);

    if (!i2sInitialized) {
        LOG_ERROR("Failed to initialize i2s library... Things will go wrong!");
    } else {
        LOG_INFO("I2S library initialized");
        cfg.logInfo();
    }
    LOG_INFO("Enabled I2S");
}

void disableBluetooth() {
    LOG_DEBUG("Disabling I2S...");
    i2s.end();
    LOG_INFO("Disabled I2S");
}

void enableRadio() {
    LOG_DEBUG("Switching radio ON...");
    display->displayLine(SWITCHING_RADIO_ON, 2, dabradio::CENTER);
    dab.setCallback(dabServiceDataCallback);
    dab.mute(true, true); // Avoid "tuning" noises
    dab.speaker(SPEAKER_NONE);
    dab.begin(1); // Actual mode set by the AudioSource
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

void switchSource(const int fromSourceIdx, const int toSourceIdx) {
    dabradio::AudioSource *toSource = sources[toSourceIdx];
    dabradio::AudioSource *fromSource = nullptr;
    if (fromSourceIdx >= 0) {
        fromSource = sources[fromSourceIdx];
    }

    if (fromSource != nullptr) {
        fromSource->deactivate();
    }

    // Tuning CPU Clock
    if (fromSource == nullptr || toSource->needsLowCpuFrequency != fromSource->needsLowCpuFrequency) {
        const long frequency = toSource->needsLowCpuFrequency ? LOW_CPU_CLOCK_MHZ : HIGH_CPU_CLOCK_MHZ;
        LOG_DEBUG("Setting CPU frequency to %ldMhz...", frequency);
        Serial.flush(); // Console is mingled at lowest frequencies. Need to flush and refresh baud rate
        setCpuFrequencyMhz(frequency);
        Serial.updateBaudRate(MONITOR_SPEED);
        logCpuFrequencies();
        LOG_INFO("Set CPU frequency to %ldMhz", frequency);
    }

    // Toggle DAB: doing here to avoid on/off/on when switching from FM to DAB
    if (fromSource == nullptr || toSource->needsRadio != fromSource->needsRadio) {
        if (toSource->needsRadio) {
            enableRadio();
        } else {
            disableRadio();
        }
    }

    // Toggle Bluetooth:
    if (fromSource == nullptr || toSource->needsBluetooth != fromSource->needsBluetooth) {
        LOG_DEBUG("Switching bluetooth %s...", toSource->needsBluetooth ? "ON" : "OFF");
        if (toSource->needsBluetooth) {
            enableBluetooth();
        } else {
            disableBluetooth();
        }
        LOG_INFO("Switched bluetooth %s", toSource->needsBluetooth ? "ON" : "OFF");
    }


    display->clear();
    toSource->activate();

    currentSourceIndex = toSourceIdx;
    selectedSourceIndex = toSourceIdx;
    currentSource = sources[currentSourceIndex];


    preferences.putInt(PREVIOUS_SOURCE_KEY, currentSourceIndex);
}

void selectorClicked() {
    LOG_DEBUG("Selector clicked");
    currentSource->modePressed();
}

void selectorDoubleClicked() {
    LOG_DEBUG("Selector double-clicked");
    currentSource->modeDoublePressed();
}

void selectorLongPressStarted() {
    LOG_DEBUG("Selector long-press started");
}

void selectorLongPressStopped() {
    LOG_DEBUG("Selector long-press stopped");
}

void tuneClicked() {
    LOG_DEBUG("Tune clicked");
    currentSource->tunePressed();
}

void tuneDoubleClicked() {
    LOG_DEBUG("Tune double-clicked");
    currentSource->tuneDoublePressed();
}

void tuneLongPressStarted() {
    LOG_DEBUG("Tune long-press started");
    currentSource->tuneLongPressed();
}

void tuneLongPressStopped() {
    LOG_DEBUG("Tune long-press stopped");
    currentSource->tuneReleased();
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
    display = new dabradio::LCDDisplay(0x27, 20, 4);
    display->switchOn();
    display->displayLine("Philips BF501 Redux", 0, dabradio::LEFT);
    display->displayLine("Starting systems...", 1, dabradio::LEFT);
    display->displayLine(versionString, 2, dabradio::CENTER);

    LOG_DEBUG("Initializing audio sources...");

    dabradio::Bluetooth::bluetoothSink = &bluetoothSink;
    sources[0] = new dabradio::FMRadio(display, &dab);
    sources[1] = new dabradio::DABRadio(display, &dab);
    sources[2] = new dabradio::Bluetooth(display);

    pinMode(DAB_SPI_SLAVE_SELECT, OUTPUT);
    digitalWrite(DAB_SPI_SLAVE_SELECT, HIGH);
    SPI.begin();
    dab.speaker(SPEAKER_DIFF);


    LOG_INFO("Initialized Audio sources: ");
    for (u_int8_t i = 0; i < NB_SOURCES; i++) {
        LOG_INFO(" -> #%d: %s", i, sources[i]->name);
    }

    LOG_DEBUG("Initializing buttons...");
    selectorButton.setClickMs(BUTTON_DOUBLECLICK_DELAY_MS);
    selectorButton.attachClick(selectorClicked);
    selectorButton.attachDoubleClick(selectorDoubleClicked);
    selectorButton.attachLongPressStart(selectorLongPressStarted);
    selectorButton.attachLongPressStop(selectorLongPressStopped);

    tuneButton.setClickMs(BUTTON_DOUBLECLICK_DELAY_MS);
    tuneButton.attachClick(tuneClicked);
    tuneButton.attachDoubleClick(tuneDoubleClicked);
    tuneButton.attachLongPressStart(tuneLongPressStarted);
    tuneButton.attachLongPressStop(tuneLongPressStopped);
    LOG_INFO("Initialized buttons");

    // Restoring previous source
    currentSourceIndex = preferences.getInt(PREVIOUS_SOURCE_KEY, 0) % NB_SOURCES; // Just to make sure
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

    for (auto &source: sources) {
        source->tick();
    }

    if (currentSource->needsRadio) {
        dab.task();
    }

    int newSelectorPosition = selectorEncoder.getPosition();
    if (selectorPosition != newSelectorPosition) {
        selectorPosition = newSelectorPosition;
        if (lastSelectedSourceTime + SELECT_SOURCE_MIN_DELAY < millis()) {
            const int step = selectorEncoder.getDirection() == RotaryEncoder::Direction::CLOCKWISE ? 1 : -1;
            selectedSourceIndex = (selectedSourceIndex + step) % NB_SOURCES; // Keep in [0..nbSources]
            // When turning anti-clockwise, we want to go from the first to the last, then one before the last etc.
            selectedSourceIndex = (selectedSourceIndex < 0 ? selectedSourceIndex + NB_SOURCES : selectedSourceIndex);
            lastSelectedSourceTime = millis();

            LOG_INFO("Selecting source #%d - %s", selectedSourceIndex, sources[selectedSourceIndex]->name);
            display->clear();
            display->displayLine(sources[selectedSourceIndex]->name, 0);
        }
    }

    if (selectedSourceIndex != currentSourceIndex && (lastSelectedSourceTime + SWITCH_SOURCE_DELAY) < millis()) {
        // Switching source
        switchSource(currentSourceIndex, selectedSourceIndex);
        lastSelectedSourceTime = 0;
    }

    int newTunerPosition = tuneEncoder.getPosition();
    if (tunerPosition != newTunerPosition) {
        RotaryEncoder::Direction tuneDirection = tuneEncoder.getDirection();
        LOG_DEBUG("Tuning %s", tuneDirection == RotaryEncoder::Direction::CLOCKWISE ? "UP" : "DOWN");
        if (tuneDirection == RotaryEncoder::Direction::CLOCKWISE) {
            currentSource->tuneUp();
        } else {
            currentSource->tuneDown();
        }
        tunerPosition = newTunerPosition;
    }
}
