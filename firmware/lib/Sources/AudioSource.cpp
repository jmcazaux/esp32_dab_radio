#include <AdvancedLogger.h>
#include <AudioSource.h>
#include <Display.h>

bool AudioSource::isActive() {
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
    LOG_INFO("Activated source \"%s\"", name);
};

void AudioSource::deactivate() {
    LOG_INFO("De-activating source \"%s\"", name);
};
