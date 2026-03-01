
#include <AdvancedLogger.h>
#include <FMRadio.h>
#include <StringUtils.h>

constexpr char PREFERENCE_NAMESPACE[] = "fm_radio";
constexpr char MODE_KEY[] = "mode";
constexpr char FREQUENCY_KEY[] = "frequency";
constexpr uint16_t MIN_FM_FREQUENCY = 8750;
constexpr uint16_t MAX_FM_FREQUENCY = 10790;

void FMRadio::activate() {
    LOG_DEBUG("Activating FM source \"%s\"...", name);
    if (this->isActive()) {
        // Only refresh the display
        display->displayLine(name, 0);
        return;
    }

    preferences.begin(PREFERENCE_NAMESPACE, false);
    uint16_t previousFrequency = preferences.getInt(FREQUENCY_KEY, MIN_FM_FREQUENCY);
    previousFrequency = max(previousFrequency, MIN_FM_FREQUENCY);
    previousFrequency = min(previousFrequency, MAX_FM_FREQUENCY);
    LOG_INFO("%s restoring mode %s: %4.1fMHz", name, "MANUAL", previousFrequency / 100.0);

    // Actually activate the source
    dab->begin(1);  // FM Mode
    dab->tune(previousFrequency);

    dab->speaker(SPEAKER_DIFF);
    dab->mute(false, false);
    dab->vol(25);

    serviceInfo.frequency = previousFrequency;
    this->displayServiceInfo();

    active = true;
    LOG_INFO("Activated FM source \"%s\"", name);
};

void FMRadio::tunePressed() {
    LOG_DEBUG("%s seeking up...", name);
    if (dab->freq == MAX_FM_FREQUENCY) {
        // Resetting to beginning of FM range
        dab->tune(MIN_FM_FREQUENCY);
    };

    dab->seek(1, 0);
    LOG_INFO("Tuned to %5.2f MHz", dab->freq / 100.0);
    this->modeOrTuningChanged();
};

void FMRadio::tuneDoublePressed() {
    LOG_DEBUG("%s: seeking down...", name);
    if (dab->freq == MIN_FM_FREQUENCY) {
        // Resetting to end of FM range
        dab->tune(MAX_FM_FREQUENCY);
    };

    dab->seek(0, 0);
    LOG_INFO("Tuned to %5.2f MHz", dab->freq / 100.0);
    this->modeOrTuningChanged();
};

void FMRadio::deactivate() {
    LOG_DEBUG("De-activating \"%s\"...", name);
    active = false;
    LOG_INFO("De-activated \"%s\"", name);
};

void FMRadio::modeOrTuningChanged() {
    this->refreshInformation();
    this->savePreferences();
}

void FMRadio::displayServiceInfo() const {
    LOG_DEBUG("Displaying service information...");
    display->clear();

    char nameAndFreqBuffer[21];
    sprintf(nameAndFreqBuffer, "%-12s%5.1fMHz", serviceInfo.serviceName, serviceInfo.frequency / 100.0);

    display->displayLine(name, 0);
    display->displayLine(nameAndFreqBuffer, 1);

    if (strlen(serviceInfo.serviceData) > 0) {
        display->displayLine(serviceInfo.serviceData, 3, ROLLING_LEFT);
    }
}

void FMRadio::refreshInformation() {
    LOG_DEBUG("Loading new service information...");
    ServiceInfo newServiceInfo{};
    char buffer[128];

    dab->status();

    newServiceInfo.frequency = dab->freq;
    LOG_DEBUG(" > Frequency:      %5.1fMHz", newServiceInfo.frequency / 100.0);

    newServiceInfo.day = dab->Days;
    newServiceInfo.month = dab->Months;
    newServiceInfo.year = dab->Year;
    newServiceInfo.hour = dab->Hours;
    newServiceInfo.minute = dab->Minutes;
    LOG_DEBUG(" > Date & time:    %02d/%02d/%04d %02d:%02d",
        newServiceInfo.day, newServiceInfo.month, newServiceInfo.year,
        newServiceInfo.hour, newServiceInfo.minute);

    sprintf(buffer, "%s", dab->ps);
    sprintf(newServiceInfo.serviceName, "%s", trim(buffer));
    LOG_DEBUG(" > Service name:   \"%s\"", newServiceInfo.serviceName);

    sprintf(buffer, "%s", dab->ServiceData);
    sprintf(newServiceInfo.serviceData, "%s", trim(buffer));
    LOG_DEBUG(" > Service data:   \"%s\"", newServiceInfo.serviceData);

    newServiceInfo.signalStrength = dab->signalstrength;
    newServiceInfo.snr = dab->snr;
    LOG_DEBUG(" > Strength / SNR: %d / %d",
        newServiceInfo.signalStrength,
        newServiceInfo.snr);

    if (newServiceInfo == serviceInfo) {
        LOG_DEBUG("Service information did not change... Keeping it.");
        return;
    }

    serviceInfo.copyFrom(newServiceInfo);

    this->displayServiceInfo();
}


void FMRadio::savePreferences() {
    preferences.putString(MODE_KEY, "manual");
    preferences.putInt(FREQUENCY_KEY, serviceInfo.frequency);
}
