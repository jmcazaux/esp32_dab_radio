
#include <AdvancedLogger.h>
#include <FMRadio.h>

constexpr char PREFERENCE_NAMESPACE[] = "fm_radio";
constexpr char MODE[] = "mode";
constexpr char FREQUENCY[] = "frequency";

void FMRadio::activate() {
    LOG_DEBUG("Activating FM source \"%s\"...", name);
    if (this->isActive()) {
        // Only refresh the display
        display->displayLine(name, 0);
        return;
    }

    // Actually activate the source
    dab.begin(1);  // FM Mode
    dab.seek(1, 0);

    display->displayLine(this->name, 0);

    active = true;
    LOG_INFO("Activated FM source \"%s\"", name);
};

void FMRadio::tunePressed() {
    LOG_DEBUG("%n seeking up...", name);
    if (dab.freq == 10790) {
        // Resetting to begining of FM range
        dab.tune(static_cast<uint16_t>(8750));
    };

    dab.seek(1, 0);
    LOG_INFO("Tuned to %5.2f MHz", dab.freq / 100.0);
    this->refreshInformation();
};

void FMRadio::tuneDoublePressed() {
    LOG_DEBUG("%n: seeking down...", name);
    if (dab.freq == 8750) {
        // Resetting to end of FM range
        dab.tune(static_cast<uint16_t>(10790));
    };

    dab.seek(0, 0);
    LOG_INFO("Tuned to %5.2f MHz", dab.freq / 100.0);
    this->refreshInformation();
};

void FMRadio::deactivate() {
    LOG_DEBUG("De-activating \"%s\"...", name);
    active = false;
    LOG_INFO("De-activated \"%s\"", name);
};

void FMRadio::displayServiceInfo() const {
    LOG_DEBUG("Displaying service information...");
    display->clear();

    char freqStr[20];
    sprintf(freqStr, "%5.1f MHz", serviceInfo.frequency / 100.0);

    display->displayLine(name, 0);
    display->displayLine(freqStr, 1);
    if (strlen(serviceInfo.serviceName) > 0) {
        display->displayLine(serviceInfo.serviceName, 2);
    }
    if (strlen(serviceInfo.serviceData) > 0) {
        display->displayLine(serviceInfo.serviceData, 3);
    }
}

void FMRadio::refreshInformation() {
    LOG_DEBUG("Loading new service information...");
    ServiceInfo newServiceInfo;

    newServiceInfo.frequency = dab.freq;
    LOG_DEBUG(" > Frequency:      %5.2f MHz", newServiceInfo.frequency / 100.0);

    newServiceInfo.day = dab.Days;
    newServiceInfo.month = dab.Months;
    newServiceInfo.year = dab.Year;
    newServiceInfo.hour = dab.Hours;
    newServiceInfo.minute = dab.Minutes;
    LOG_DEBUG(" > Date & time:    %02d/%02d/%4d %02d:%02d",
        newServiceInfo.day, newServiceInfo.month, newServiceInfo.year);

    sprintf(newServiceInfo.serviceName, PSTR("%s"), dab.ps);
    LOG_DEBUG(" > Service name:   %s (%s)", newServiceInfo.serviceName, dab.ps);

    sprintf(newServiceInfo.serviceData, PSTR("%s\n"), dab.ServiceData);
    LOG_DEBUG(" > Service data:   %s (%s)", newServiceInfo.serviceData, dab.ServiceData);

    newServiceInfo.signalStrength = dab.signalstrength;
    newServiceInfo.snr = dab.snr;
    LOG_DEBUG(" > Strength / SNR: %d / %d (%d / %d)",
        newServiceInfo.signalStrength,
        newServiceInfo.snr,
        dab.signalstrength,
        dab.snr);

    if (newServiceInfo == serviceInfo) {
        LOG_DEBUG("Service information did not change... Keeping it.");
        return;
    }

    serviceInfo.copyFrom(newServiceInfo);

    this->displayServiceInfo();
}
