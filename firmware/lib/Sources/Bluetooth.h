#pragma once

#include <AudioSource.h>
#include <Display.h>

static const char BLUETOOTH[] = "Bluetooth";

class Bluetooth : public AudioSource {
   public:
    Bluetooth(Display* display) : AudioSource(BLUETOOTH, false, true, false, display) {};
};
