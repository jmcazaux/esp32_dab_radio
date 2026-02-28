#pragma once

#include <AudioSource.h>
#include <DABShield.h>
#include <Display.h>

static const char FM_RADIO[] = "Radio FM";

class FMRadio : public AudioSource {
   public:
    FMRadio(Display* display, DAB* dab) : AudioSource(FM_RADIO, true, false, true, display), dab(dab) {};

    void activate() override;
    void deactivate() override;

    void tunePressed() override;
    void tuneDoublePressed() override;

   private:
    DAB* dab;

    static void serviceData();
};
