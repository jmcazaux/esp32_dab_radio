#include <Bluetooth.h>
#include <AdvancedLogger.h>
#include <ArduinoJson.h>
#include <esp_a2dp_api.h>
#include <StringUtils.h>

#include <SourceStrings.h>
#include <SourceConstants.h>
#include <esp_log.h>

#include "../../.pio/libdeps/debug/ESP32-A2DP/src/BluetoothA2DPSink.h"

namespace com::ironbird::esp32dabradio {
    constexpr char LOG_TAG[] = "E32DR";

    void Bluetooth::onConnectionStateChanged(esp_a2d_connection_state_t state, void *obj) {
        ESP_LOGD("Connections state changed: %d", state);
        auto *instance = static_cast<Bluetooth *>(obj);
        instance->connectionStateChanged(state);
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

    void Bluetooth::connectionStateChanged(const esp_a2d_connection_state_t state) {
        switch (state) {
            case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
                serviceInfo.state = ServiceInfo::State::NOT_CONNECTED;
                break;
            case ESP_A2D_CONNECTION_STATE_CONNECTING:
                serviceInfo.state = ServiceInfo::State::CONNECTING;
                break;
            case ESP_A2D_CONNECTION_STATE_CONNECTED:
                serviceInfo.state = ServiceInfo::State::CONNECTED;
                strncpy(serviceInfo.peerName, bluetoothSink->get_peer_name(), 31);
                break;
            case ESP_A2D_CONNECTION_STATE_DISCONNECTING:
                serviceInfo.state = ServiceInfo::State::CONNECTED;
                break;
        }
        displayInformation();
        ESP_LOGW(LOG_TAG, "Connections state change registered: %d", static_cast<int>(serviceInfo.state));
    }


    void Bluetooth::activate() {
        displayName();
        if (active) {
            ESP_LOGD(LOG_TAG, "%s already active... Skipping activation.");
            return;
        }

        ESP_LOGD(LOG_TAG, "Activating source %s...", name);

        ESP_LOGD(LOG_TAG, "Setting callback %p", this);
        bluetoothSink->set_on_connection_state_changed(onConnectionStateChanged, this);
        bluetoothSink->set_peer_name_callback(onPeerNameChanged);
        bluetoothSink->set_avrc_metadata_callback(onAVRCMetadataChanged);
        bluetoothSink->set_avrc_rn_play_pos_callback(onPlayPositionChanged, 1);

        bluetoothSink->start(BLUETOOTH_NAME);
        ESP_LOGI(LOG_TAG, "Bluetooth AD2P service advertised as \"%s\"", BLUETOOTH_NAME);

        displayInformation();

        ESP_LOGI(LOG_TAG, "Activated source \"%s\"", name);
    }

    void Bluetooth::deactivate() {
        bluetoothSink->stop();
        active = false;
    }

    void Bluetooth::displayInformation() {
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
                display->clearLine(1);
                display->clearLine(2);
                display->clearLine(3);
                break;
        }
        ESP_LOGI(LOG_TAG, "Displayed %s service information (%d ms)...", name, millis() - now);
    }
}
