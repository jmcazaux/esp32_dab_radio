#include <Bluetooth.h>
#include <AdvancedLogger.h>
#include <ArduinoJson.h>
#include <StringUtils.h>

#include <SourceStrings.h>
#include <SourceConstants.h>

namespace com::ironbird::esp32dabradio {
    void Bluetooth::activate() {
        LOG_DEBUG("Activating source %s...", name);
        bluetoothSink->start(BLUETOOTH_NAME);
    }

    void Bluetooth::deactivate() {
        bluetoothSink->stop();
    }
}
