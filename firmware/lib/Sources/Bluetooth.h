#pragma once

#include <AudioSource.h>
#include <Display.h>
#include <LocalizedStrings.h>


class Bluetooth : public AudioSource {
public:
    Bluetooth(Display *display) : AudioSource(SOURCE_BLUETOOTH, false, true, false, display) {
    };
};
