#include <AdvancedLogger.h>
#include <AudioSource.h>
#include <Display.h>

bool AudioSource::isActive() const {
    return active;
};

void AudioSource::activate() {
    LOG_DEBUG("Activating source \"%s\"...", name);
    if (this->isActive()) {
        // Only refresh the display
        display->displayLine(name, 0);
        return;
    }

    // Actually activate the source
    display->displayLine(this->name, 0);

    active = true;
    LOG_INFO("Activated source \"%s\"", name);
};

void AudioSource::deactivate() {
    LOG_DEBUG("De-activating source \"%s\"...", name);
    active = false;
    LOG_INFO("De-activated source \"%s\"", name);
};
