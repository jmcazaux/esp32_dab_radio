#pragma once

namespace com::ironbird::esp32dabradio {
    // Audio sources
    constexpr char SOURCE_FM_RADIO[] = "FM Radio";
    constexpr char SOURCE_DAB_RADIO[] = "DAB Radio";
    constexpr char SOURCE_BLUETOOTH[] = "Bluetooth";
    constexpr char BLUETOOTH_NAME[] = "Philips BF501";

    // Modes
    constexpr char MODE_MANUAL[] = "MAN";
    constexpr char MODE_LIST[] = "LIST";
    constexpr char MODE_MEMORY[] = "MEM";

    // Actions
    constexpr char REFRESHING_PRESETS[] = "Refreshing presets";
    constexpr char PRESET_ID_AND_FREQ[] = "> %02d : %.1fMHz";
    constexpr char PRESET_ID_AND_NAME[] = "> %02d : %s";
    constexpr char MEMORIZING_FREQ[] = ">Saving %.1fMHz...";
    constexpr char MEMORIZING_NAME[] = ">Saving %s...";
    constexpr char MEMORY_PRESET_ID_NAME_AND_FREQ[] = "->M%02d %s %.1f";
    constexpr char MEMORY_PRESET_ID_ONLY[] = "->M%02d";
    constexpr char MEMORY_PRESET_ID_AND_NAME[] = "->M%02d %s";
    constexpr char RELEASE_TO_STORE[] = "Release to store";
    constexpr char CONNECT_TO[] = "Connect to:";
    constexpr char CONNECTING[] = "Connecting...";
    constexpr char DISCONNECTING[] = "Disconnecting...";
    constexpr char CONNECTED[] = "Connected";
    constexpr char STOPPED[] = "Stopped";
    constexpr char PAUSED[] = "Paused";
}
