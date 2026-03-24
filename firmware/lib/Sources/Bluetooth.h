#pragma once

#include <AudioSource.h>
#include <Display.h>
#include <esp_a2dp_api.h>
#include <mutex>
#include <SourceStrings.h>
#include <SourceConstants.h>

#include "BluetoothA2DPSink.h"


class BluetoothA2DPSink;

namespace com::ironbird::esp32dabradio {
    class Bluetooth : public AudioSource {
    public:
        static BluetoothA2DPSink *bluetoothSink;

        Bluetooth(Display *display) : AudioSource(
            SOURCE_BLUETOOTH, false, true, false,
            display) {
        }


        void activate() override;


        void deactivate() override;


        void displayInformation() override;

    private:
        class ServiceInfo {
        public:
            enum class State {
                NOT_CONNECTED,
                CONNECTING,
                DISCONNECTING,
                CONNECTED,
            };



            enum class PlayStatus {
                PLAYING,
                PAUSED,
                STOPPED
            };



            State state = State::NOT_CONNECTED;
            PlayStatus playStatus = PlayStatus::STOPPED;

            char peerName[SERVICE_INFO_NAME_LENGTH + 1] = "";
            char album[SERVICE_INFO_DATA_LENGTH + 1] = "";
            char artist[SERVICE_INFO_DATA_LENGTH + 1] = "";
            char track[SERVICE_INFO_DATA_LENGTH + 1] = "";

            unsigned long playPosition = 0;
            unsigned long trackLength = 0;

            unsigned int trackNumber = 0;
            unsigned int numberOfTracks = 0;


            bool operator==(const ServiceInfo &other) const;


            void copyFrom(const ServiceInfo &other);


            void clear();
        };



        std::mutex mutex;

        ServiceInfo serviceInfo{};


        void displayServiceConnectedInfo() const;


        void displayServiceInfo() const;


        void displayServiceInfoIfNeeded(ServiceInfo &newServiceInfo);


        void displayName() const;


        static void onConnectionStateChanged(esp_a2d_connection_state_t state, void *obj);


        static void onPeerNameChanged(char *peerName);


        static void onAVRCMetadataChanged(uint8_t, const uint8_t *value);


        static void onPlayPositionChanged(uint32_t playPos);


        static void onPlayStatusChanged(esp_avrc_playback_stat_t rawPlayStatus);


        void setPlayPosition(uint32_t playPos);


        void setPeerName(char *name);


        void setConnectionState(ServiceInfo::State state);


        void setAVRCMetadata(uint8_t metadata, char *value);


        void setPlayStatus(ServiceInfo::PlayStatus playStatus);


        static ServiceInfo::State mapConnectionState(esp_a2d_connection_state_t state);


        static ServiceInfo::PlayStatus mapPlayStatus(esp_avrc_playback_stat_t playStatus);
    };
}
