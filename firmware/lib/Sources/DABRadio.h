#pragma once

#include <AudioSource.h>
#include <DABShield.h>
#include <Display.h>

static const char DAB_RADIO[] = "Radio DAB+";

class DABRadio : public AudioSource {
   public:
    DABRadio(Display* display, DAB* dab) : AudioSource(DAB_RADIO, true, false, true, display), dab(dab) {};

   private:
    DAB* dab;
};
