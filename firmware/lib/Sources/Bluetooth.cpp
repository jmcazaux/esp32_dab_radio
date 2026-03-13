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
    void Bluetooth::onConnectionStateChanged(esp_a2d_connection_state_t state, void *obj) {
        LOG_DEBUG("Connections state changed: %d", state);
        auto *instance = static_cast<Bluetooth *>(obj);
        instance->connectionStateChanged(state);
    }

    static void onPeerNameChanged(char *peer_name) {
        ESP_LOGW("E32DR", "Peer name changed: %s", peer_name);
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

        ESP_LOGW("E32DR", "Connections state change registered: %d", static_cast<int>(serviceInfo.state));
    }


    void Bluetooth::activate() {
        displayName();
        if (active) {
            LOG_DEBUG("%s already active... Skipping activation.");
            return;
        }

        LOG_DEBUG("Activating source %s...", name);

        LOG_DEBUG("Setting callback %p", this);
        bluetoothSink->set_on_connection_state_changed(onConnectionStateChanged, this);
        bluetoothSink->set_peer_name_callback(onPeerNameChanged);

        bluetoothSink->start(BLUETOOTH_NAME);
        LOG_INFO("Bluetooth AD2P service advertised as \"%s\"", BLUETOOTH_NAME);
        LOG_INFO("Activated source \"%s\"", name);
    }

    void Bluetooth::deactivate() {
        bluetoothSink->stop();
    }
}
