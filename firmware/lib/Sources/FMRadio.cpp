
#include <AdvancedLogger.h>
#include <FMRadio.h>
#include <StringUtils.h>
#include <ArduinoJson.h>

#include <SourceStrings.h>
#include <SourceConstants.h>

constexpr char PREFERENCE_NAMESPACE[] = "fm_radio";
constexpr char MODE_KEY[] = "mode";
constexpr char FREQUENCY_KEY[] = "frequency";
constexpr char LIST_PRESET_KEY[] = "list_preset";
constexpr char MEMORY_PRESET_KEY[] = "memory_preset";

constexpr char PRESET_FILE[] = "/fm_radio_presets.json";
constexpr char LIST_PRESETS_JSON_KEY[] = "listPresets";
constexpr char MEMORY_PRESETS_JSON_KEY[] = "memoryPresets";
constexpr char NAME_JSON_KEY[] = "name";
constexpr char FREQUENCY_JSON_KEY[] = "frequency";

constexpr int MEMORY_PRESETS_SIZE = 10;

constexpr uint16_t MIN_FM_FREQUENCY = 8750;
constexpr uint16_t MAX_FM_FREQUENCY = 10790;

constexpr unsigned long CHANGE_FREQUENCY_DELAY_MS = 400;
constexpr unsigned long LARGER_FREQUENCY_STEP_DELAY_MS = 100;

constexpr uint16_t MIN_FREQUENCY_STEP_CHANGE = 10;
constexpr uint16_t LARGE_FREQUENCY_STEP_CHANGE = 10 * 10;

enum Mode {
    MANUAL,
    LIST,
    MEMORY
};

static const char *MODE_NAMES[] = {MODE_MANUAL, MODE_LIST, MODE_MEMORY};

void FMRadio::displayNameAndMode() const {
    display->displayJustified(name, MODE_NAMES[currentMode], 0);
}

void FMRadio::activate() {
    LOG_DEBUG("Activating FM source \"%s\"...", name);
    if (this->isActive()) {
        // Only refresh the display
        displayNameAndMode();
        return;
    }
    LOG_INFO("Restoring presets...");
    loadPresets();

    // Restoring previous mode & frequency
    preferences.begin(PREFERENCE_NAMESPACE, false);
    const uint8_t previousMode = preferences.getInt(MODE_KEY, currentMode);
    currentMode = min(static_cast<int>(previousMode), static_cast<int>(std::size(MODE_NAMES)) - 1);
    uint16_t previousFrequency = preferences.getInt(FREQUENCY_KEY, MIN_FM_FREQUENCY);
    previousFrequency = max(previousFrequency, MIN_FM_FREQUENCY);
    previousFrequency = min(previousFrequency, MAX_FM_FREQUENCY);

    currentPresetIndex = min(
        static_cast<int>(listPresets.size() - 1),
        max(0, preferences.getInt(LIST_PRESET_KEY, currentPresetIndex))
    );

    currentMemoryIndex = min(
        static_cast<int>(memoryPresets.size() - 1),
        max(0, preferences.getInt(MEMORY_PRESET_KEY, currentMemoryIndex))
    );

    LOG_INFO("%s restoring mode %s (MAN=%.1fMHz, LIST=%d, MEM=%d)",
             name, MODE_NAMES[currentMode],
             previousFrequency / 100.0, currentPresetIndex, currentMemoryIndex
    );
    displayNameAndMode();

    // Actually activate the source
    dab->mute(true, true);
    dab->begin(1); // FM Mode

    if (currentMode == MANUAL) {
        dab->tune(previousFrequency);
        serviceInfo.frequency = previousFrequency;
    } else if (currentMode == LIST) {
        tunePreset(listPresets[currentPresetIndex]);
    } else if (currentMode == MEMORY) {
        tunePreset(memoryPresets[currentMemoryIndex]);
    }

    dab->speaker(SPEAKER_DIFF);
    dab->vol(35);
    dab->mute(false, false);

    this->modeOrTuningChanged();

    active = true;
    LOG_INFO("Activated FM source \"%s\"", name);
}

void FMRadio::tick() {
    if (targetFrequency != dab->freq
        && lastTargetFrequencyChange + CHANGE_FREQUENCY_DELAY_MS < millis()) {
        LOG_DEBUG("Tuning to %5.1fMHz...", targetFrequency / 100.0);
        dab->tune(targetFrequency);
        modeOrTuningChanged();
    }
}

void FMRadio::refreshListPresets() {
    LOG_DEBUG("Refreshing list presets...");
    display->displayLine(REFRESHING_PRESETS, 1, CENTER);
    display->clearLine(2);
    display->clearLine(3);

    const uint16_t currentFrequency = dab->freq;
    dab->mute(true, true);
    char idAndFreqBuffer[32];

    listPresets.clear();

    dab->tune(MIN_FM_FREQUENCY);
    while (dab->seek(1, 0)) {
        dab->status();
        if (dab->freq == MAX_FM_FREQUENCY)
            break;

        // Save the preset
        auto newPreset = Preset{};
        newPreset.frequency = dab->freq;
        listPresets.emplace_back(newPreset);
        LOG_DEBUG("Found stations at %.1fMHz. Saving as preset #%d", newPreset.frequency / 100.0, listPresets.size());

        // Display what we have found and progress
        sprintf(idAndFreqBuffer, PRESET_ID_AND_FREQ, listPresets.size(), newPreset.frequency / 100.0);
        display->displayLine(idAndFreqBuffer, 2);

        const auto progress = static_cast<uint8_t>(round(
            100.0 * (newPreset.frequency - MIN_FM_FREQUENCY) / (MAX_FM_FREQUENCY - MIN_FM_FREQUENCY)));
        display->displayProgress(progress, 3);
        LOG_DEBUG("New preset #%d - %.1fMHz (%d%%)", listPresets.size(), newPreset.frequency / 100.0, progress);
    }

    display->displayProgress(100, 3);

    // Resetting everything
    dab->tune(currentFrequency);
    dab->mute(false, false);
    delay(500);
    display->clearLine(1);
    display->clearLine(2);
    display->clearLine(3);
    displayServiceInfo();
    LOG_INFO("Refreshed list presets (found %d stations)... Saving.", listPresets.size());
    savePresets();
}

void FMRadio::offsetTargetFrequency(const uint16_t frequencyInc) {
    const uint16_t newTarget = targetFrequency + frequencyInc;
    targetFrequency = min(max(newTarget, MIN_FM_FREQUENCY), MAX_FM_FREQUENCY);
    LOG_DEBUG("Targeting %5.1fMHz...", targetFrequency / 100.0);
    lastTargetFrequencyChange = millis();
    displayServiceInfo();
}

void FMRadio::tunePreset(const Preset &preset) {
    dab->tune(preset.frequency);
    serviceInfo.frequency = preset.frequency;
    strncpy(serviceInfo.serviceName, preset.name, 32);
}


void FMRadio::tuneFrequency(const TuneDirection direction) {
    const uint16_t frequencyStep =
            direction * (millis() - lastTargetFrequencyChange < LARGER_FREQUENCY_STEP_DELAY_MS
                             ? LARGE_FREQUENCY_STEP_CHANGE
                             : MIN_FREQUENCY_STEP_CHANGE);

    LOG_DEBUG("Tuning up by %.1fMHz (delay %dms)",
              frequencyStep / 100.0,
              millis() - lastTargetFrequencyChange);
    offsetTargetFrequency(frequencyStep);
    lastTargetFrequencyChange = millis();
}


void FMRadio::tuneList(const TuneDirection direction) {
    if (currentPresetIndex == 0 && direction == TUNE_DOWN) {
        currentPresetIndex = listPresets.size() - 1;
    } else {
        currentPresetIndex = (currentPresetIndex + direction) % listPresets.size();
    }
    LOG_DEBUG("Tuning to list preset %d: %.1fMHz",
              currentPresetIndex,
              listPresets[currentPresetIndex].frequency / 100.0);

    dab->tune(listPresets[currentPresetIndex].frequency);
    modeOrTuningChanged();
}

void FMRadio::tuneMemory(TuneDirection direction) {
    if (currentMemoryIndex == 0 && direction == TUNE_DOWN) {
        currentMemoryIndex = memoryPresets.size() - 1;
    } else {
        currentMemoryIndex = (currentMemoryIndex + direction) % listPresets.size();
    }
    LOG_DEBUG("Tuning to memory preset %d: %.1fMHz",
              currentMemoryIndex,
              memoryPresets[currentMemoryIndex].frequency / 100.0);

    dab->tune(memoryPresets[currentMemoryIndex].frequency);
    modeOrTuningChanged();
}

void FMRadio::selectTargetMemoryPreset(TuneDirection direction) {
    if (targetMemoryPreset == 0 && direction == TUNE_DOWN) {
        targetMemoryPreset = memoryPresets.size() - 1;
    } else {
        targetMemoryPreset = (targetMemoryPreset + direction) % memoryPresets.size();
    }
    LOG_DEBUG("Selected target memory preset %d", targetMemoryPreset);
    displayServiceInfo();
}


void FMRadio::tuneUp() {
    if (memorizingPreset) {
        selectTargetMemoryPreset(TUNE_UP);
    } else if (currentMode == MANUAL) {
        tuneFrequency(TUNE_UP);
    } else if (currentMode == LIST) {
        tuneList(TUNE_UP);
    } else if (currentMode == MEMORY) {
        tuneMemory(TUNE_UP);
    }
}

void FMRadio::tuneDown() {
    if (memorizingPreset) {
        selectTargetMemoryPreset(TUNE_DOWN);
    } else if (currentMode == MANUAL) {
        tuneFrequency(TUNE_DOWN);
    } else if (currentMode == LIST) {
        tuneList(TUNE_DOWN);
    } else if (currentMode == MEMORY) {
        tuneMemory(TUNE_DOWN);
    }
}

void FMRadio::tunePressed() {
    LOG_DEBUG("%s seeking up...", name);
    if (dab->freq == MAX_FM_FREQUENCY) {
        // Resetting to beginning of FM range
        dab->tune(MIN_FM_FREQUENCY);
    };

    dab->seek(1, 0);
    LOG_INFO("Tuned to %5.2fMHz", dab->freq / 100.0);
    this->modeOrTuningChanged();
};

void FMRadio::tuneDoublePressed() {
    LOG_DEBUG("%s: seeking down...", name);
    if (dab->freq == MIN_FM_FREQUENCY) {
        // Resetting to end of FM range
        dab->tune(MAX_FM_FREQUENCY);
    };

    dab->seek(0, 0);
    LOG_INFO("Tuned to %5.2fMHz", dab->freq / 100.0);
    this->modeOrTuningChanged();
}

void FMRadio::tuneLongPressed() {
    LOG_INFO("Entering memorizing mode...");
    memorizingPreset = true;
    displayServiceInfo();
}

void FMRadio::tuneReleased() {
    LOG_INFO("Exiting memorizing mode...");
    memorizingPreset = false;

    memoryPresets[targetMemoryPreset].frequency = dab->freq;
    strncpy(memoryPresets[targetMemoryPreset].name, serviceInfo.serviceName, 32);
    targetMemoryPreset = 0;
    displayServiceInfo();
    savePresets();
}

void FMRadio::modePressed() {
    currentMode = (currentMode + 1) % std::size(MODE_NAMES);
    displayNameAndMode();
}

void FMRadio::modeDoublePressed() {
    refreshListPresets();
}

void FMRadio::deactivate() {
    LOG_DEBUG("De-activating \"%s\"...", name);
    active = false;
    LOG_INFO("De-activated \"%s\"", name);
}

void FMRadio::modeOrTuningChanged() {
    targetFrequency = dab->freq;

    if (currentMode == LIST && listPresets.empty()) {
        LOG_INFO("Preset lise is empty... Triggering refresh.");
        refreshListPresets();
    }

    strcpy(serviceInfo.serviceName, "");
    strcpy(serviceInfo.serviceData, "");

    this->refreshInformation();
    this->savePreferences();
}

void FMRadio::displayStandardInfo() {
    char freqBuffer[21];
    sprintf(freqBuffer, "%.1fMHz", dab->freq / 100.0);

    char nameBuffer[21];

    // In case we have pressed 'TUNE' to seek up (or down) we do not want to show that we are on a preset
    const bool isManuallyTuned =
            (currentMode == LIST && dab->freq != listPresets[currentPresetIndex].frequency)
            || (currentMode == MEMORY && dab->freq != memoryPresets[currentMemoryIndex].frequency);

    if (currentMode == MANUAL || isManuallyTuned) {
        strcpy(nameBuffer, serviceInfo.serviceName);
    } else if (currentMode == LIST) {
        sprintf(nameBuffer, "#%d %s", currentPresetIndex + 1, serviceInfo.serviceName);
    } else if (currentMode == MEMORY) {
        sprintf(nameBuffer, "M%02d %s", currentMemoryIndex + 1, serviceInfo.serviceName);
    }

    display->displayJustified(nameBuffer, freqBuffer, 1);
    display->clearLine(2);

    if (strlen(serviceInfo.serviceData) > 0) {
        display->displayLine(serviceInfo.serviceData, 3, ROLLING_LEFT);
    } else {
        display->clearLine(3);
    }
}

void FMRadio::displayInfoInMemorizingMode() {
    char buffer[21];
    auto [targetPresetFrequency, targetPresetName] = memoryPresets[targetMemoryPreset];

    sprintf(buffer, MEMORIZING_FREQ, dab->freq / 100.0);
    display->displayLine(buffer, 1);

    if (targetPresetFrequency == MIN_FM_FREQUENCY && strlen(targetPresetName) == 0) {
        // We would override something
        sprintf(buffer, MEMORY_PRESET_ID_ONLY, targetMemoryPreset + 1);
    } else {
        // Nothing stored here yey
        sprintf(buffer, MEMORY_PRESET_ID_AND_DETAILS, targetMemoryPreset + 1, targetPresetName,
                targetPresetFrequency / 100.0);
    }
    display->displayLine(buffer, 2);

    display->displayLine(RELEASE_TO_STORE, 3, CENTER);
}

void FMRadio::displayInfoInManualTuningMode() {
    // We are tuning the frequency
    char freqBuffer[21];
    sprintf(freqBuffer, ">>> %.1fMHz", targetFrequency / 100.0);
    display->displayLine(freqBuffer, 1, RIGHT);
    if (strlen(serviceInfo.serviceData) > 0) {
        strcpy(serviceInfo.serviceData, "");
    }
}

void FMRadio::displayServiceInfo() {
    LOG_DEBUG("Displaying service information...");

    if (targetFrequency != dab->freq) {
        displayInfoInManualTuningMode();
    } else if (memorizingPreset) {
        displayInfoInMemorizingMode();
    } else {
        displayStandardInfo();
    }

    LOG_INFO("Displayed service information");
}

void FMRadio::updatePresetsServiceName(const ServiceInfo &info) {
    for (auto &[frequency, name]: listPresets) {
        if (frequency == info.frequency && strcmp(name, info.serviceName) != 0) {
            LOG_DEBUG("Updating name of preset at %.1fMHz", frequency / 100.0);
            strncpy(name, info.serviceName, 32);

            savePresets();
        }
    }
}

String FMRadio::presetsAsJson() {
    JsonDocument doc;

    // Serialize the list presets
    const auto jsonListPresets = doc[LIST_PRESETS_JSON_KEY].to<JsonArray>();
    const auto jsonMemoryPresets = doc[MEMORY_PRESETS_JSON_KEY].to<JsonArray>();

    for (auto &[presetFrequency, presetName]: listPresets) {
        auto preset = jsonListPresets.add<JsonObject>();
        preset[NAME_JSON_KEY] = presetName;
        preset[FREQUENCY_JSON_KEY] = presetFrequency;
    }

    for (auto &[presetFrequency, presetName]: memoryPresets) {
        auto preset = jsonMemoryPresets.add<JsonObject>();
        preset[NAME_JSON_KEY] = presetName;
        preset[FREQUENCY_JSON_KEY] = presetFrequency;
    }
    String output;
    serializeJson(doc, output);

    return output;
}


void FMRadio::loadPresetsFromJson(String jsonString) {
    JsonDocument doc;
    const DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        LOG_ERROR(
            "Got \"%s\" error while deserializing presets from the below JSON:\n---\n%s\n---",
            error.c_str(),
            jsonString.c_str()
        );
        LOG_ERROR("Presets wont' be deserialized...");
        return;
    }

    // Loading list presets
    listPresets.clear(); // Most certainly useless, but yet.
    const auto jsonListPresets = doc[LIST_PRESETS_JSON_KEY].as<JsonArray>();

    for (JsonObject preset: jsonListPresets) {
        auto newPreset = Preset{};
        strncpy(newPreset.name, preset[NAME_JSON_KEY], 32);
        newPreset.frequency = preset[FREQUENCY_JSON_KEY];
        listPresets.emplace_back(newPreset);
    }

    // Loading memory presets
    memoryPresets.clear();
    const auto jsonMemoryPresets = doc[MEMORY_PRESETS_JSON_KEY].as<JsonArray>();

    for (JsonObject preset: jsonMemoryPresets) {
        auto newPreset = Preset{};
        strncpy(newPreset.name, preset[NAME_JSON_KEY], 32);
        newPreset.frequency = preset[FREQUENCY_JSON_KEY];
        memoryPresets.emplace_back(newPreset);
    }

    LOG_INFO("Presets loaded: %d list presets, %d memory presets", listPresets.size(), memoryPresets.size());
}

void FMRadio::refreshInformation() {
    LOG_DEBUG("Loading new service information...");
    ServiceInfo newServiceInfo{};
    char buffer[128];

    dab->status();

    newServiceInfo.frequency = dab->freq;

    newServiceInfo.day = dab->Days;
    newServiceInfo.month = dab->Months;
    newServiceInfo.year = dab->Year;
    newServiceInfo.hour = dab->Hours;
    newServiceInfo.minute = dab->Minutes;

    sprintf(buffer, "%s", dab->ps);
    sprintf(newServiceInfo.serviceName, "%s", trim(buffer));

    sprintf(buffer, "%s", dab->ServiceData);
    sprintf(newServiceInfo.serviceData, "%s", trim(buffer));

    newServiceInfo.signalStrength = dab->signalstrength;
    newServiceInfo.snr = dab->snr;

    LOG_DEBUG(" > Frequency:      %.1fMHz", newServiceInfo.frequency / 100.0);
    LOG_DEBUG(" > Date & time:    %02d/%02d/%04d %02d:%02d",
              newServiceInfo.day, newServiceInfo.month, newServiceInfo.year,
              newServiceInfo.hour, newServiceInfo.minute);
    LOG_DEBUG(" > Service name:   \"%s\"", newServiceInfo.serviceName);
    LOG_DEBUG(" > Service data:   \"%s\"", newServiceInfo.serviceData);
    LOG_DEBUG(" > Strength / SNR: %d / %d",
              newServiceInfo.signalStrength,
              newServiceInfo.snr);

    if (newServiceInfo == serviceInfo) {
        LOG_DEBUG("Service information did not change... Keeping it.");
        return;
    }

    serviceInfo.copyFrom(newServiceInfo);
    if (strlen(serviceInfo.serviceName) == 0) {
        // Maybe we have a service name in the presets
        if (
            const char *serviceName = getServiceNameFromPresets(dab->freq);
            serviceName != nullptr
        ) {
            strcpy(serviceInfo.serviceName, serviceName);
        }
    }
    this->displayServiceInfo();

    if (strlen(newServiceInfo.serviceName) > 0) {
        // We need to update the presets
        updatePresetsServiceName(newServiceInfo);
    }
}

char *FMRadio::getServiceNameFromPresets(const uint16_t frequency) {
    for (auto &[presetFrequency, presetName]: listPresets) {
        if (presetFrequency == frequency) {
            return presetName;
        }
    }

    return nullptr;
}

void FMRadio::savePreferences() {
    LOG_DEBUG("Saving preferences...");
    preferences.putInt(MODE_KEY, currentMode);
    if (currentMode == MANUAL) {
        preferences.putInt(FREQUENCY_KEY, serviceInfo.frequency);
    } else if (currentMode == LIST) {
        preferences.putInt(LIST_PRESET_KEY, currentPresetIndex);
    } else if (currentMode == MEMORY) {
        preferences.putInt(MEMORY_PRESET_KEY, currentMemoryIndex);
    }
    LOG_INFO("Saved preferences");
}

void FMRadio::loadPresets() {
    LOG_DEBUG("Loading presets from %s...", PRESET_FILE);
    File file = LittleFS.open(PRESET_FILE, FILE_READ);
    if (!file) {
        LOG_ERROR("Failed to open preset file %s for read... Presets won't be loaded.", PRESET_FILE);
        return;
    }
    String jsonString = "";
    while (file.available()) {
        jsonString += static_cast<char>(file.read());
    }
    file.close();
    Serial.println("---Presets file---");
    Serial.println(jsonString);
    Serial.println("------------------");

    loadPresetsFromJson(jsonString);

    for (auto i = memoryPresets.size(); i < MEMORY_PRESETS_SIZE; i++) {
        memoryPresets.emplace_back(Preset{});
    }

    LOG_INFO("Loaded presets from %s: %d list presets", PRESET_FILE, listPresets.size());
}


void FMRadio::savePresets() {
    const String jsonString = presetsAsJson();
    Serial.println("---JSON presets---");
    Serial.println(jsonString);
    Serial.println("------------------");

    File file = LittleFS.open(PRESET_FILE, FILE_WRITE);
    if (!file) {
        LOG_ERROR("Failed to open preset file %s for write... Presets won't be saved.", PRESET_FILE);
        return;
    }

    if (file.print(jsonString)) {
        LOG_INFO("Saved presets (%d list presets)...", listPresets.size());
    } else {
        LOG_ERROR("Failed to write presets to file %s for write... Presets won't be saved.", PRESET_FILE);
        return;
    }

    file.close();
}

bool FMRadio::ServiceInfo::operator==(const FMRadio::ServiceInfo &other) const {
    if (this->frequency != other.frequency) return false;

    if (this->year != other.year) return false;
    if (this->month != other.month) return false;
    if (this->day != other.day) return false;
    if (this->hour != other.hour) return false;
    if (this->minute != other.minute) return false;

    if (this->signalStrength != other.signalStrength) return false;
    if (this->snr != other.snr) return false;

    if (strcmp(this->serviceName, other.serviceName) != 0) return false;
    if (strcmp(this->serviceData, other.serviceData) != 0) return false;

    return true;
}

void FMRadio::ServiceInfo::copyFrom(const ServiceInfo &other) {
    this->frequency = other.frequency;

    this->year = other.year;
    this->month = other.month;
    this->day = other.day;
    this->hour = other.hour;
    this->minute = other.minute;

    this->signalStrength = other.signalStrength;
    this->snr = other.snr;

    strcpy(this->serviceName, other.serviceName);
    strcpy(this->serviceData, other.serviceData);
}

void FMRadio::ServiceInfo::clear() {
    this->frequency = 0;
    this->signalStrength = 0;
    this->snr = 0;
    strcpy(this->serviceName, "");
    strcpy(this->serviceData, "");
}


