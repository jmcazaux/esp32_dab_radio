#pragma once

#include <AudioSource.h>
#include <Display.h>
#include <SourceStrings.h>

#include "BluetoothA2DPSink.h"


namespace com::ironbird::esp32dabradio {
    class Bluetooth : public AudioSource {
    public:
        Bluetooth(Display *display, BluetoothA2DPSink *bluetoothSink) : AudioSource(
                                                                            SOURCE_BLUETOOTH, false, true, false,
                                                                            display), bluetoothSink(bluetoothSink) {
        }

        void activate() override;

        void deactivate() override;

    private:
        BluetoothA2DPSink *bluetoothSink;
    };
}
