#pragma once

#include <AdvancedLogger.h>
#include <Display.h>

class AudioSource {
public:
    virtual ~AudioSource() = default;

    const char *name;
    const bool needsRadio = false;
    const bool needsBluetooth = false;
    const bool needsLowCpuFrequency = false;

    Display *display;

    virtual void tick() {
        // Intentionally blank. Override in concrete classes.
    };

    virtual void tuneUp() {
        // Intentionally blank. Override in concrete classes.
    };

    virtual void tuneDown() {
        // Intentionally blank. Override in concrete classes.
    };

    virtual void tunePressed() {
        // Intentionally blank. Override in concrete classes.
    };

    virtual void tuneLongPressed() {
        // Intentionally blank. Override in concrete classes.
    };

    virtual void tuneReleased() {
        // Intentionally blank. Override in concrete classes.
    };

    virtual void tuneDoublePressed() {
        // Intentionally blank. Override in concrete classes.
    };

    virtual void modePressed() {
        // Intentionally blank. Override in concrete classes.
    };

    virtual void modeDoublePressed() {
        // Intentionally blank. Override in concrete classes.
    };

    AudioSource(
        const char name[],
        const bool needsRadio,
        const bool needsBluetooth,
        const bool needsLowCpuFrequency,
        Display *display) : name(name),
                            needsRadio(needsRadio),
                            needsBluetooth(needsBluetooth),
                            needsLowCpuFrequency(needsLowCpuFrequency),
                            display(display) {
    };

    bool isActive() const;

    // Activate the audio source
    virtual void activate();

    // Deactivate the audio source
    virtual void deactivate();

    // Refresh information about the current audio stream and refresh the display
    // TODO: There should be no actual need to call this externally, but the callback function
    //       for the DAB service data has to be static and therefore can only be declared in `main.cpp`.
    virtual void refreshInformation() {
    };

protected:
    bool active = false;
};
