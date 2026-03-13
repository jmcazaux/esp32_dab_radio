#pragma once

#include <AudioSource.h>
#include <Display.h>
#include <esp_a2dp_api.h>
#include <SourceStrings.h>

#include "BluetoothA2DPSink.h"


class BluetoothA2DPSink;

namespace com::ironbird::esp32dabradio {
    class Bluetooth : public AudioSource {
    public:
        Bluetooth(Display *display, BluetoothA2DPSink *bluetoothSink) : AudioSource(
                                                                            SOURCE_BLUETOOTH, false, true, false,
                                                                            display), bluetoothSink(bluetoothSink) {
        }

        void activate() override;

        void deactivate() override;

        void connectionStateChanged(esp_a2d_connection_state_t state);

    private:
        class ServiceInfo {
        public:
            enum class State {
                NOT_CONNECTED,
                CONNECTING,
                CONNECTED,
            };

            char peerName[64] = "";
            State state = State::NOT_CONNECTED;
        };

        BluetoothA2DPSink *bluetoothSink;

        ServiceInfo serviceInfo{};

        static void onConnectionStateChanged(esp_a2d_connection_state_t state, void *obj);

        void displayName() const;
    };
}
