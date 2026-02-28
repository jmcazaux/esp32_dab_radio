
#include <AdvancedLogger.h>
#include <FMRadio.h>

void FMRadio::serviceData() {
    LOG_DEBUG("Got service data");
    char statusstring[72];

    /*
    sprintf_P(statusstring, PSTR("%02d/%02d/%04d "), dab.Days, dab.Months, dab.Year);
    Serial.print(statusstring);
    sprintf_P(statusstring, PSTR("%02d:%02d "), dab.Hours, dab.Minutes);
    Serial.print(statusstring);
    sprintf_P(statusstring, PSTR("%s "), dab.ps);
    Serial.print(statusstring);
    sprintf_P(statusstring, PSTR("%s\n"), dab.ServiceData);
    Serial.print(statusstring);
    */

}

void FMRadio::activate() {
    LOG_DEBUG("Activating FM source \"%s\"...", name);
    if (this->isActive()) {
        // Only refresh the display
        display->displayLine(name, 0);
        return;
    }

    // Actually activate the source
    dab->begin(1);  // FM Mode
    dab->seek(1, 0);

    display->displayLine(this->name, 0);

    active = true;
    LOG_INFO("Activated FM source \"%s\"", name);
};

void FMRadio::tunePressed() {
    LOG_DEBUG("%n: seeking up...");
    if (dab->freq == 10790) {
        // Resetting to begining of FM range
        dab->tune((uint16_t)8750);
    };

    dab->seek(1, 0);
    LOG_INFO("Tuned to %5.2f MHz", dab->freq / 100.0);
};

void FMRadio::tuneDoublePressed() {
    LOG_DEBUG("%n: seeking down...");
    if (dab->freq == 8750) {
        // Resetting to end of FM range
        dab->tune((uint16_t)10790);
    };

    dab->seek(0, 0);
    LOG_INFO("Tuned to %5.2f MHz", dab->freq / 100.0);
};

void FMRadio::deactivate() {
    LOG_DEBUG("De-activating FM source \"%s\"...", name);
    active = false;
    LOG_INFO("De-activated FM source \"%s\"", name);
};
