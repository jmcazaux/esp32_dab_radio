#pragma once

#include <Display.h>
#include <AdvancedLogger.h>

class AudioSource {
   public:
    const char* name;
    const bool needsRadio = false;
    const bool needsBluetooth = false;
    const bool needsLowCpuFrequency = false;

    Display* display;

    virtual void tuneUp() { LOG_DEBUG("Source \"%s\" tuning up.", name); };
    virtual void tuneDown() { LOG_DEBUG("Source \"%s\" tuning down.", name); };
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

    bool isActive();
    virtual void deactivate();
    virtual void activate();

   protected:
    bool active = false;
};
