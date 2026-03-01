#pragma once

#include <AdvancedLogger.h>
#include <Display.h>

class AudioSource {
   public:
    virtual ~AudioSource() = default;

    const char* name;
    const bool needsRadio = false;
    const bool needsBluetooth = false;
    const bool needsLowCpuFrequency = false;

    Display* display;

    virtual void tuneUp() {};
    virtual void tuneDown() {};
    virtual void tunePressed() {};
    virtual void tuneLongPressed() {};
    virtual void tuneDoublePressed() {};

    AudioSource(
        const char name[],
        const bool needsRadio,
        const bool needsBluetooth,
        const bool needsLowCpuFrequency,
        Display* display) : name(name),
                            needsRadio(needsRadio),
                            needsBluetooth(needsBluetooth),
                            needsLowCpuFrequency(needsLowCpuFrequency),
                            display(display) {};

    bool isActive() const;
    // Activate the audio source
    virtual void activate();
    // Deactivate the audio source
    virtual void deactivate();
    // Refresh information about the current audio stream and refresh the display
    // TODO: There should be no actual need to call this externally, but the callback function
    //       for the DAB service data has to be static and therefore can only be declared in `main.cpp`.
    virtual void refreshInformation() {};

   protected:
    bool active = false;
};
