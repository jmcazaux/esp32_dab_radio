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

    void modePressed() override;

    void modeDoublePressed() override;

private:
    DAB *dab;
    Preferences preferences;
    unsigned long lastTargetFrequencyChange = 0;
    uint16_t targetFrequency = 0;
    uint8_t currentMode = 0;

    uint8_t currentPresetIndex = 0;

    void refreshListPresets();

    void offsetTargetFrequency(uint16_t frequencyInc);

    void tuneFrequency(TuneDirection direction);

    void tuneList(TuneDirection direction);

    void modeOrTuningChanged();

    void displayServiceInfo();

    void savePreferences();

    char *getServiceNameFromPresets(uint16_t frequency);

    struct ServiceInfo {
        uint16_t frequency;

        char serviceName[64];
        char serviceData[128];

        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;

        int8_t signalStrength;
        int8_t snr;

        // TODO: the below would rather be in the cpp file, but failed to do this
        bool operator==(const ServiceInfo &other) const {
            if (this->frequency != other.frequency) return false;

            if (this->year != other.year) return false;
            if (this->month != other.month) return false;
            if (this->day != other.day) return false;
            if (this->hour != other.hour) return false;
            if (this->minute != other.minute) return false;

            if (this->signalStrength != other.signalStrength) return false;
            if (this->snr != other.snr) return false;

            if (strcmp(this->serviceName, other.serviceName) != 0) return false;
            if (strcmp(this->serviceData, other.serviceData) != 0) return false;

            return true;
        };

        void copyFrom(const ServiceInfo &other) {
            this->frequency = other.frequency;

            this->year = other.year;
            this->month = other.month;
            this->day = other.day;
            this->hour = other.hour;
            this->minute = other.minute;

            this->signalStrength = other.signalStrength;
            this->snr = other.snr;

            strcpy(this->serviceName, other.serviceName);
            strcpy(this->serviceData, other.serviceData);
        };

        void clear() {
            this->frequency = 0;
            this->signalStrength = 0;
            this->snr = 0;
            strcpy(this->serviceName, "");
            strcpy(this->serviceData, "");
        };
    };

    ServiceInfo serviceInfo{};

    struct Preset {
        uint16_t frequency = 0;
        char name[32] = "";
    };

    std::vector<Preset> listPresets;

    void updatePresetsServiceName(const ServiceInfo &serviceInfo);
};
