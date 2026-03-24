#include <Bluetooth.h>
#include <ArduinoJson.h>
#include <esp_a2dp_api.h>
#include <mutex>

#include <SourceStrings.h>
#include <SourceConstants.h>

#include "BluetoothA2DPSink.h"


namespace com::ironbird::esp32dabradio {
    constexpr char LOG_TAG[] = "E32DR";

    BluetoothA2DPSink *Bluetooth::bluetoothSink = nullptr;

    void Bluetooth::onConnectionStateChanged(esp_a2d_connection_state_t state, void *obj) {
        ESP_LOGD(LOG_TAG, "Connections state changed: %d", state);
        auto *instance = static_cast<Bluetooth *>(obj);
        auto connectionState = mapConnectionState(state);
        instance->setConnectionState(connectionState);
    }

    void Bluetooth::onPeerNameChanged(char *peerName) {
        char buffer[SERVICE_INFO_NAME_LENGTH + 1];
        strncpy(buffer, peerName, SERVICE_INFO_NAME_LENGTH);
        ESP_LOGD(LOG_TAG, "Peer name changed \"%s\"", buffer);

        auto *instance = static_cast<Bluetooth *>(bluetoothSink->get_reference());
        instance->setPeerName(buffer);
    }

    void Bluetooth::onAVRCMetadataChanged(uint8_t parameter, const uint8_t *value) {
        char buffer[SERVICE_INFO_DATA_LENGTH + 1];
        strncpy(buffer, (char *) value, SERVICE_INFO_DATA_LENGTH);
        ESP_LOGD(LOG_TAG, "AVRC metadata changed 0x%x - \"%s\"", parameter, buffer);

        auto *instance = static_cast<Bluetooth *>(bluetoothSink->get_reference());
        instance->setAVRCMetadata(parameter, buffer);
    }

    void Bluetooth::onPlayPositionChanged(uint32_t playPos) {
        ESP_LOGD(LOG_TAG, "Play position changed %d ms", playPos);
        auto *instance = static_cast<Bluetooth *>(bluetoothSink->get_reference());
        instance->setPlayPosition(playPos);
    }

    void Bluetooth::onPlayStatusChanged(esp_avrc_playback_stat_t rawPlayStatus) {
        auto playStatus = mapPlayStatus(rawPlayStatus);
        ESP_LOGD(LOG_TAG, "Play status changed 0x%x (source 0x%x)", internalPlayStatus, playStatus);
        auto *instance = static_cast<Bluetooth *>(bluetoothSink->get_reference());
        instance->setPlayStatus(playStatus);
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
        bluetoothSink->set_refernce(this);
        bluetoothSink->set_on_connection_state_changed(onConnectionStateChanged, this);
        bluetoothSink->set_peer_name_callback(onPeerNameChanged);
        bluetoothSink->set_avrc_metadata_callback(onAVRCMetadataChanged);
        bluetoothSink->set_avrc_rn_play_pos_callback(onPlayPositionChanged, 1);
        bluetoothSink->set_avrc_rn_playstatus_callback(onPlayStatusChanged);

        bluetoothSink->start(BLUETOOTH_NAME);
        ESP_LOGI(LOG_TAG, "Bluetooth AD2P service advertised as \"%s\"", BLUETOOTH_NAME);

        mutex.lock();
        serviceInfo.clear();
        serviceInfo.trackLength = 1; // Force redisplay of activation
        mutex.unlock();
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


    void Bluetooth::displayServiceConnectedInfo() const {
        display->displayJustified(name, CONNECTED, 0);
        display->displayLine(serviceInfo.peerName, 1, CENTER);

        // Artist + album
        char buffer[SERVICE_INFO_DATA_LENGTH * 2 + 4];
        if (strlen(serviceInfo.artist) & strlen(serviceInfo.album)) {
            sprintf(buffer, "%s - %s", serviceInfo.artist, serviceInfo.album);
        } else {
            // Only either is filled
            sprintf(buffer, "%s%s", serviceInfo.artist, serviceInfo.album);
        }
        display->displayLine(buffer, 2, ROLLING_LEFT);

        switch (serviceInfo.playStatus) {
            case ServiceInfo::PlayStatus::PLAYING:
                // Track
                if (serviceInfo.numberOfTracks > 1 || serviceInfo.trackNumber > 1) {
                    sprintf(buffer, "%s - %d/%d", serviceInfo.track, serviceInfo.trackNumber,
                            serviceInfo.numberOfTracks);
                } else {
                    sprintf(buffer, "%s", serviceInfo.track);
                }
                display->displayLine(buffer, 3, ROLLING_LEFT);
                break;
            case ServiceInfo::PlayStatus::PAUSED:
                display->displayLine(PAUSED, 3, CENTER);
                break;
            case ServiceInfo::PlayStatus::STOPPED:
                display->displayLine(STOPPED, 3, CENTER);
                break;
        }
    }


    void Bluetooth::displayServiceInfo() const {
        ESP_LOGD(LOG_TAG, "Displaying %s service information...", name);
        const unsigned long now = millis();
        switch (serviceInfo.state) {
            case ServiceInfo::State::NOT_CONNECTED: {
                displayName();
                display->displayLine(CONNECT_TO, 1, CENTER);
                display->displayLine(BLUETOOTH_NAME, 2, CENTER);
                display->clearLine(3);
                break;
            }
            case ServiceInfo::State::CONNECTING:
                displayName();
                display->displayLine(CONNECTING, 1, CENTER);
                display->clearLine(2);
                display->clearLine(3);
                break;
            case ServiceInfo::State::DISCONNECTING:
                displayName();
                display->displayLine(DISCONNECTING, 1, CENTER);
                display->clearLine(2);
                display->clearLine(3);
                break;
            case ServiceInfo::State::CONNECTED:
                displayServiceConnectedInfo();
                break;
        }
        ESP_LOGI(LOG_TAG, "Displayed %s service information (%d ms)...", name, millis() - now);
    }


    void Bluetooth::setPlayPosition(uint32_t playPos) {
        ServiceInfo newServiceInfo;

        newServiceInfo.copyFrom(serviceInfo);
        newServiceInfo.playPosition = playPos;

        displayServiceInfoIfNeeded(newServiceInfo);
    }


    void Bluetooth::setPeerName(char *name) {
        ServiceInfo newServiceInfo;

        newServiceInfo.copyFrom(serviceInfo);
        strncpy(newServiceInfo.peerName, name, SERVICE_INFO_NAME_LENGTH);

        displayServiceInfoIfNeeded(newServiceInfo);
    }

    void Bluetooth::setConnectionState(ServiceInfo::State state) {
        ServiceInfo newServiceInfo;

        newServiceInfo.copyFrom(serviceInfo);
        newServiceInfo.state = state;

        displayServiceInfoIfNeeded(newServiceInfo);
    }

    void Bluetooth::setAVRCMetadata(uint8_t metadata, char *value) {
        ESP_LOGD(LOG_TAG, "Displaying AVRC metadata 0x%x - \"%s\"", metadata, value);
        auto newServiceInfo = ServiceInfo{};
        newServiceInfo.copyFrom(serviceInfo);

        switch (metadata) {
            case ESP_AVRC_MD_ATTR_TITLE:
                strncpy(newServiceInfo.track, value, SERVICE_INFO_DATA_LENGTH);
                break;
            case ESP_AVRC_MD_ATTR_ARTIST:
                strncpy(newServiceInfo.artist, value, SERVICE_INFO_DATA_LENGTH);
                break;

            case ESP_AVRC_MD_ATTR_ALBUM:
                strncpy(newServiceInfo.album, value, SERVICE_INFO_DATA_LENGTH);
                break;

            case ESP_AVRC_MD_ATTR_TRACK_NUM:
                newServiceInfo.trackNumber = std::stoi(value);
                break;

            case ESP_AVRC_MD_ATTR_NUM_TRACKS:
                newServiceInfo.numberOfTracks = std::stoi(value);
                break;

            case ESP_AVRC_MD_ATTR_GENRE:
            case ESP_AVRC_MD_ATTR_PLAYING_TIME:
                // These are not handled yet...
                break;

            default:
                ESP_LOGW(LOG_TAG, "Unknown metadata 0x%x - \"%s\"", metadata, value);
        }

        displayServiceInfoIfNeeded(newServiceInfo);
    }

    void Bluetooth::setPlayStatus(ServiceInfo::PlayStatus playStatus) {
        auto newServiceInfo = ServiceInfo{};
        newServiceInfo.copyFrom(serviceInfo);
        newServiceInfo.playStatus = playStatus;
        displayServiceInfoIfNeeded(newServiceInfo);
    }

    void Bluetooth::displayServiceInfoIfNeeded(ServiceInfo &newServiceInfo) {
        // State and peer name callbacks might be a bit racy on startup
        // Making sure we have the right state & peer name
        newServiceInfo.state = mapConnectionState(bluetoothSink->get_connection_state());
        strncpy(newServiceInfo.peerName, bluetoothSink->get_peer_name(), SERVICE_INFO_NAME_LENGTH);

        if (newServiceInfo == serviceInfo) {
            ESP_LOGI(LOG_TAG, "Service information did not change... Keeping it.");
            return;
        }

        ESP_LOGI(LOG_TAG, "Displaying new service info...");
        ESP_LOGI(LOG_TAG, " > Connected state: %d", newServiceInfo.state);
        ESP_LOGI(LOG_TAG, " > Peer name:       \"%s\"", newServiceInfo.peerName);
        ESP_LOGI(LOG_TAG, " > Mode:            %d", newServiceInfo.playStatus);
        ESP_LOGI(LOG_TAG, " > Artist:          %s", newServiceInfo.artist);
        ESP_LOGI(LOG_TAG, " > Album:           %s", newServiceInfo.album);
        ESP_LOGI(LOG_TAG, " > Track:           %s", newServiceInfo.track);
        ESP_LOGI(LOG_TAG, " > Track number:    %d", newServiceInfo.trackNumber);
        ESP_LOGI(LOG_TAG, " > Nb. of tracks:   %d", newServiceInfo.numberOfTracks);
        ESP_LOGI(LOG_TAG, " > Track length:    %d", newServiceInfo.trackLength);
        ESP_LOGI(LOG_TAG, " > Play position:   %d", newServiceInfo.playPosition);

        mutex.lock();
        serviceInfo.copyFrom(newServiceInfo);
        displayServiceInfo();
        mutex.unlock();
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

        displayServiceInfoIfNeeded(newServiceInfo);
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
                return ServiceInfo::State::DISCONNECTING;
        }
        return ServiceInfo::State::NOT_CONNECTED;
    }

    Bluetooth::ServiceInfo::PlayStatus Bluetooth::mapPlayStatus(esp_avrc_playback_stat_t playStatus) {
        switch (playStatus) {
            case ESP_AVRC_PLAYBACK_STOPPED:
                return ServiceInfo::PlayStatus::STOPPED;
            case ESP_AVRC_PLAYBACK_PLAYING:
            case ESP_AVRC_PLAYBACK_FWD_SEEK:
            case ESP_AVRC_PLAYBACK_REV_SEEK:
                return ServiceInfo::PlayStatus::PLAYING;
            case ESP_AVRC_PLAYBACK_PAUSED:
                return ServiceInfo::PlayStatus::PAUSED;
            case ESP_AVRC_PLAYBACK_ERROR:
                return ServiceInfo::PlayStatus::STOPPED;
        }
        return ServiceInfo::PlayStatus::STOPPED;
    }

    bool Bluetooth::ServiceInfo::operator==(const ServiceInfo &other) const {
        if (this->numberOfTracks != other.numberOfTracks) return false;
        if (this->trackNumber != other.trackNumber) return false;
        if (this->playPosition != other.playPosition) return false;
        if (this->trackLength != other.trackLength) return false;
        if (this->state != other.state) return false;
        if (this->playStatus != other.playStatus) return false;
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
        this->playStatus = other.playStatus;
        strcpy(this->peerName, other.peerName);
        strcpy(this->album, other.album);
        strcpy(this->artist, other.artist);
        strcpy(this->track, other.track);
    }


    void Bluetooth::ServiceInfo::clear() {
        this->numberOfTracks = 0;
        this->trackNumber = 0;
        this->playPosition = 0;
        this->trackLength = 0;
        this->state = State::NOT_CONNECTED;
        this->playStatus = PlayStatus::STOPPED;
        strcpy(this->peerName, "");
        strcpy(this->album, "");
        strcpy(this->artist, "");
        strcpy(this->track, "");
    }
}
