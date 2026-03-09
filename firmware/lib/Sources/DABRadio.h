#pragma once

#include <AudioSource.h>
#include <DABShield.h>
#include <Display.h>
#include <SourceStrings.h>

#include "SourceConstants.h"
#include "ArduinoJson/Array/JsonArray.hpp"


class DABRadio : public AudioSource {
public:
    DABRadio(Display *display, DAB *dab) : AudioSource(SOURCE_DAB_RADIO, true, false, true, display), dab(dab) {
    };

    void activate() override;

    void tick() override;

    void tuneUp() override;

    void tuneDown() override;

    void modePressed();

    void tuneLongPressed() override;

    void tuneReleased() override;

    void modeDoublePressed() override;

    void displayInformation() override;

private:
    class ServiceInfo {
    public:
        uint8_t frequencyIndex = 0;
        uint32_t serviceId = 0;
        uint32_t compId = 0;

        char serviceName[64] = "";
        char serviceData[128] = "";

        uint16_t year = 0;
        uint8_t month = 0;
        uint8_t day = 0;
        uint8_t hour = 0;
        uint8_t minute = 0;


        uint16_t bitRate = 0;
        uint16_t sampleRate = 0;
        boolean dabPlus = false;
        int8_t signalStrength = 0;
        int8_t snr = 0;
        uint8_t quality = 0;

        bool operator==(const ServiceInfo &other) const;

        void copyFrom(const ServiceInfo &other);

        void clear();
    };

    struct Preset {
        uint8_t dabEnsemble = 0;
        uint32_t serviceId = 0;
        uint32_t compId = 0;
        char name[32] = "";
    };

    static bool presetComparator(const Preset &lhs, const Preset &rhs);

    DAB *dab;
    Preferences preferences;
    ServiceInfo serviceInfo{};

    unsigned long lastDabStatusRefresh = 0;

    // The tune button goes up / down but not stable yet
    unsigned long lastTargetPresetChange = 0;
    uint16_t targetPresetIndex = 0;

    bool memorizingPreset = false;
    uint8_t targetMemoryPreset = 0; // Memory preset where
    std::vector<Preset> listPresets;
    std::vector<Preset> memoryPresets;
    uint8_t currentMode = 0;
    uint8_t currentListIndex = 0;
    uint8_t currentMemoryIndex = 0;

    [[nodiscard]] uint8_t getCurrentModePresetIndex() const;

    std::vector<Preset> getCurrentModePresets();

    void refreshListPresets();

    void tunePreset(Preset preset);

    bool isTuning() const;

    void changeTargetPreset(TuneDirection direction);

    void selectTargetMemoryPreset(TuneDirection direction);

    void displayNameAndMode() const;

    void displayServiceInfo();

    void modeOrTuningChanged();

    void displayStandardServiceInfo();

    void displayTuningServiceInfo();

    void displayMemorizingServiceInfo();

    __gnu_cxx::__alloc_traits<std::allocator<DABRadio::Preset> >::value_type getCurrentPreset();

    static Preset getPresetFromJson(ArduinoJson::JsonObject preset);

    void loadPresetsFromJson(const String &jsonString);

    void loadPresets();


    static void addPresetToJsonArray(ArduinoJson::JsonArray presetsArray, Preset preset);

    String presetsAsJson() const;

    void savePresets() const;

    void savePreferences();
};
