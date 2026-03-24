#pragma once

namespace com::ironbird::esp32dabradio {
    enum TuneDirection {
        TUNE_UP = 1,
        TUNE_DOWN = -1,
    };

    constexpr uint8_t PRESET_NAME_LENGTH = 64;
    constexpr uint8_t SERVICE_INFO_NAME_LENGTH = 64;
    constexpr uint8_t SERVICE_INFO_DATA_LENGTH = 128;
}
