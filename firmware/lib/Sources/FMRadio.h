#pragma once

#include <AudioSource.h>
#include <DABShield.h>
#include <Display.h>
#include <SourceStrings.h>
#include <SourceConstants.h>
#include <vector>

class FMRadio : public AudioSource {
public:
    FMRadio(Display *display, DAB *dab) : AudioSource(SOURCE_FM_RADIO, true, false, true, display), dab(dab) {
    };

    void displayNameAndMode() const;

    void activate() override;

    void deactivate() override;

    void refreshInformation() override;


    void tick() override;


    void tuneUp() override;

    void tuneDown() override;

    void tunePressed() override;

    void tuneDoublePressed() override;

    void tuneLongPressed() override;

    void tuneReleased() override;

    void modePressed() override;

    void modeDoublePressed() override;

private:
    class ServiceInfo {
    public:
        uint16_t frequency = 0;

        char serviceName[64] = "";
        char serviceData[128] = "";

        uint16_t year = 0;
        uint8_t month = 0;
        uint8_t day = 0;
        uint8_t hour = 0;
        uint8_t minute = 0;

        int8_t signalStrength = 0;
        int8_t snr = 0;

        bool operator==(const ServiceInfo &other) const;

        void copyFrom(const ServiceInfo &other);

        void clear();
    };

    struct Preset {
        uint16_t frequency = 8750;
        char name[32] = "";
    };

    DAB *dab;
    Preferences preferences;
    ServiceInfo serviceInfo{};

    std::vector<Preset> listPresets;
    std::vector<Preset> memoryPresets;
    uint8_t currentMode = 0;
    uint8_t currentPresetIndex = 0;
    uint8_t currentMemoryIndex = 0;

    unsigned long lastTargetFrequencyChange = 0;
    uint16_t targetFrequency = 0;

    bool memorizingPreset = false; // True when we are in the process of memorizing a preset
    uint8_t targetMemoryPreset = 0; // Memory preset where

    void refreshListPresets();

    void offsetTargetFrequency(uint16_t frequencyInc);

    void tuneFrequency(TuneDirection direction);

    void tuneList(TuneDirection direction);

    void tuneMemory(TuneDirection direction);

    void tunePreset(const Preset &preset);

    void selectTargetMemoryPreset(TuneDirection direction);

    void modeOrTuningChanged();

    void displayStandardInfo();

    void displayInfoInMemorizingMode();

    void displayInfoInManualTuningMode();

    void displayServiceInfo();

    void savePreferences();

    void loadPresets();

    void savePresets();

    String presetsAsJson();

    void loadPresetsFromJson(String jsonString);

    char *getServiceNameFromPresets(uint16_t frequency);

    void updatePresetsServiceName(const ServiceInfo &serviceInfo);
};
