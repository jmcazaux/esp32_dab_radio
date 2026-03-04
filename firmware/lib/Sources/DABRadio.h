#pragma once

#include <AudioSource.h>
#include <DABShield.h>
#include <Display.h>
#include <LocalizedStrings.h>


class DABRadio : public AudioSource {
public:
    DABRadio(Display *display, DAB *dab) : AudioSource(SOURCE_DAB_RADIO, true, false, true, display), dab(dab) {
    };

    void deactivate() override;

    void activate() override;

private:
    DAB *dab;
};
