#include <Bluetooth.h>
#include <AdvancedLogger.h>
#include <ArduinoJson.h>
#include <esp_a2dp_api.h>
#include <mutex>

#include <SourceStrings.h>
#include <SourceConstants.h>

#include "BluetoothA2DPSink.h"

namespace com::ironbird::esp32dabradio {
    constexpr char LOG_TAG[] = "E32DR";

    void Bluetooth::onConnectionStateChanged(esp_a2d_connection_state_t state, void *obj) {
        ESP_LOGD(LOG_TAG, "Connections state changed: %d", state);
    }

    void Bluetooth::onPeerNameChanged(char *peer_name) {
        char buffer[SERVICE_INFO_NAME_LENGTH + 1];
        strncpy(buffer, peer_name, SERVICE_INFO_NAME_LENGTH);

        ESP_LOGD(LOG_TAG, "Peer name changed \"%s\"", buffer);
    }

    void Bluetooth::onAVRCMetadataChanged(uint8_t, const uint8_t *) {
    }

    void Bluetooth::onPlayPositionChanged(uint32_t play_pos) {
    }

    void Bluetooth::displayName() const {
        display->displayLine(name, 0);
    }

    void Bluetooth::activate() {
        displayName();
        if (active) {
            ESP_LOGD(LOG_TAG, "%s already active... Skipping activation.");
            return;
        }

        ESP_LOGD(LOG_TAG, "Activating source %s...", name);

        ESP_LOGD(LOG_TAG, "Setting callback %p", this);

        bluetoothSink->start(BLUETOOTH_NAME);
        ESP_LOGI(LOG_TAG, "Bluetooth AD2P service advertised as \"%s\"", BLUETOOTH_NAME);

        serviceInfo.clear();
        serviceInfo.trackLength = 1; // Force redisplay of activation
        displayInformation();

        ESP_LOGI(LOG_TAG, "Activated source \"%s\"", name);
    }

    void Bluetooth::deactivate() {
        if (bluetoothSink->is_connected()) {
            bluetoothSink->set_connected(false);
        }
        bluetoothSink->stop();
        active = false;
        serviceInfo.clear();
    }

    void Bluetooth::displayServiceInfo() const {
        ESP_LOGD(LOG_TAG, "Displaying %s service information...", name);
        unsigned long now = millis();
        switch (serviceInfo.state) {
            case ServiceInfo::State::NOT_CONNECTED: {
                display->displayLine(CONNECT_TO, 1, CENTER);
                display->displayLine(BLUETOOTH_NAME, 2, CENTER);
                display->clearLine(3);
                break;
            }
            case ServiceInfo::State::CONNECTING:
                display->displayLine(CONNECTING, 1, CENTER);
                display->clearLine(2);
                display->clearLine(3);
                break;
            case ServiceInfo::State::CONNECTED:
                display->displayJustified(name, CONNECTED, 0);
                display->displayLine(serviceInfo.peerName, 1, CENTER);
                display->clearLine(2);
                display->clearLine(3);
                break;
        }
        ESP_LOGI(LOG_TAG, "Displayed %s service information (%d ms)...", name, millis() - now);
    }

    void Bluetooth::displayInformation() {
        ESP_LOGD(LOG_TAG, "Refreshing %s service information...", name);
        ServiceInfo newServiceInfo;
        newServiceInfo.state = mapConnectionState(bluetoothSink->get_connection_state());
        if (newServiceInfo.state == ServiceInfo::State::CONNECTED) {
            strncpy(newServiceInfo.peerName, bluetoothSink->get_peer_name(), SERVICE_INFO_NAME_LENGTH);
        } else {
            newServiceInfo.peerName[0] = '\0';
        }

        ESP_LOGI(LOG_TAG, "Got service info...");
        ESP_LOGD(LOG_TAG, " > Connected state: %d", newServiceInfo.state);
        ESP_LOGD(LOG_TAG, " > Peer name:       \"%s\"", newServiceInfo.peerName);
        ESP_LOGD(LOG_TAG, " > Mode:            %d", newServiceInfo.mode);

        if (newServiceInfo == serviceInfo) {
            ESP_LOGI(LOG_TAG, "Service information did not change... Keeping it.");
            return;
        }
        mutex.lock();
        serviceInfo.copyFrom(newServiceInfo);
        displayServiceInfo();
        mutex.unlock();
    }

    Bluetooth::ServiceInfo::State Bluetooth::mapConnectionState(const esp_a2d_connection_state_t state) {
        switch (state) {
            case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
                return ServiceInfo::State::NOT_CONNECTED;
            case ESP_A2D_CONNECTION_STATE_CONNECTING:
                return ServiceInfo::State::CONNECTING;
            case ESP_A2D_CONNECTION_STATE_CONNECTED:
                return ServiceInfo::State::CONNECTED;
            case ESP_A2D_CONNECTION_STATE_DISCONNECTING:
                return ServiceInfo::State::NOT_CONNECTED;
        }
        return ServiceInfo::State::NOT_CONNECTED;
    }

    bool Bluetooth::ServiceInfo::operator==(const ServiceInfo &other) const {
        if (this->numberOfTracks != other.numberOfTracks) return false;
        if (this->trackNumber != other.trackNumber) return false;
        if (this->playPosition != other.playPosition) return false;
        if (this->trackLength != other.trackLength) return false;
        if (this->state != other.state) return false;
        if (this->mode != other.mode) return false;
        if (strcmp(this->peerName, other.peerName) != 0) return false;
        if (strcmp(this->album, other.album) != 0) return false;
        if (strcmp(this->artist, other.artist) != 0) return false;
        if (strcmp(this->track, other.track) != 0) return false;

        return true;
    }

    void Bluetooth::ServiceInfo::copyFrom(const ServiceInfo &other) {
        this->numberOfTracks = other.numberOfTracks;
        this->trackNumber = other.trackNumber;
        this->playPosition = other.playPosition;
        this->trackLength = other.trackLength;
        this->state = other.state;
        this->mode = other.mode;
        strcpy(this->peerName, other.peerName);
        strcpy(this->album, other.album);
        strcpy(this->artist, other.artist);
        strcpy(this->track, other.track);
        Serial.print("<<<<");
        Serial.print(this->peerName);
        Serial.println(">>>>");
    }


    void Bluetooth::ServiceInfo::clear() {
        this->numberOfTracks = 0;
        this->trackNumber = 0;
        this->playPosition = 0;
        this->trackLength = 0;
        this->state = State::NOT_CONNECTED;
        this->mode = Mode::STOPPED;
        strcpy(this->peerName, "");
        strcpy(this->album, "");
        strcpy(this->artist, "");
        strcpy(this->track, "");
    }
}
