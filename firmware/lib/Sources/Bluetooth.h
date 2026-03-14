#pragma once

#include <AudioSource.h>
#include <Display.h>
#include <esp_a2dp_api.h>
#include <SourceStrings.h>

#include "BluetoothA2DPSink.h"
#include "SourceConstants.h"


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

        void displayInformation() override;

        // Used for BluetoothAD2P call backs
        void connectionStateChanged(esp_a2d_connection_state_t state);

    private:
        class ServiceInfo {
        public:
            enum class State {
                NOT_CONNECTED,
                CONNECTING,
                CONNECTED,
            };

            enum class Mode {
                PLAYING,
                PAUSED,
                STOPPED
            };

            State state = State::NOT_CONNECTED;
            Mode mode = Mode::STOPPED;

            char peerName[SERVICE_INFO_NAME_LENGTH + 1] = "";
            char album[SERVICE_INFO_DATA_LENGTH + 1] = "";
            char artist[SERVICE_INFO_DATA_LENGTH + 1] = "";
            char track[SERVICE_INFO_DATA_LENGTH + 1] = "";

            unsigned long playPosition = 0;
            unsigned long trackLength = 0;

            unsigned int trackNumber = 0;
            unsigned long numberOfTracks = 0;
        };

        BluetoothA2DPSink *bluetoothSink;

        ServiceInfo serviceInfo{};

        static void onConnectionStateChanged(esp_a2d_connection_state_t state, void *obj);

        static void onPeerNameChanged(char *peer_name);

        static void onAVRCMetadataChanged(uint8_t, const uint8_t *);

        static void onPlayPositionChanged(uint32_t play_pos);

        void displayName() const;
    };
}
