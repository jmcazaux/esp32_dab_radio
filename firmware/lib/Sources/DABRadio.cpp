#include <DABRadio.h>
#include <AdvancedLogger.h>
#include <ArduinoJson.h>
#include <StringUtils.h>

#include <SourceStrings.h>
#include <SourceConstants.h>

constexpr char PREFERENCE_NAMESPACE[] = "dab_radio";
constexpr char MODE_KEY[] = "mode";
constexpr char LIST_PRESET_KEY[] = "list_preset";
constexpr char MEMORY_PRESET_KEY[] = "memory_preset";

constexpr char PRESET_FILE[] = "/dab_radio_presets.json";
constexpr char LIST_PRESETS_JSON_KEY[] = "listPresets";
constexpr char MEMORY_PRESETS_JSON_KEY[] = "memoryPresets";
constexpr char NAME_JSON_KEY[] = "name";
constexpr char ENSEMBLE_ID_JSON_KEY[] = "ensembleId";
constexpr char SERVICE_ID_JSON_KEY[] = "serviceId";
constexpr char COMP_ID_JSON_KEY[] = "compId";

constexpr int MEMORY_PRESETS_SIZE = 10;

constexpr unsigned long CHANGE_PRESET_DELAY_MS = 1000; // At 10Mhz might be able to go lower at 40MHz
constexpr long DAB_STATUS_REFRESH_DELAY = 5000; // 5 seconds

enum class DABMode {
    LIST,
    MEMORY
};

static const char *MODE_NAMES[] = {MODE_LIST, MODE_MEMORY};

bool DABRadio::presetComparator(const Preset &lhs, const Preset &rhs) {
    return strcmp(lhs.name, rhs.name) < 0;
}


void DABRadio::activate() {
    LOG_DEBUG("Activating source \"%s\"...", name);
    if (this->isActive()) {
        // Only refresh the display
        displayNameAndMode();
        return;
    }

    if (listPresets.empty() && memoryPresets.empty()) {
        LOG_INFO("Restoring presets...");
        loadPresets();
    }

    // Restoring previous mode & frequency
    preferences.begin(PREFERENCE_NAMESPACE, false);
    const uint8_t previousMode = preferences.getInt(MODE_KEY, currentMode);
    currentMode = min(static_cast<int>(previousMode), static_cast<int>(std::size(MODE_NAMES)) - 1);

    currentListIndex = min(
        static_cast<int>(listPresets.size() - 1),
        max(0, preferences.getInt(LIST_PRESET_KEY, currentListIndex))
    );

    currentMemoryIndex = min(
        static_cast<int>(memoryPresets.size() - 1),
        max(0, preferences.getInt(MEMORY_PRESET_KEY, currentMemoryIndex))
    );

    LOG_INFO("%s restoring mode %s (LIST=%d, MEM=%d)",
             name, MODE_NAMES[currentMode], currentListIndex, currentMemoryIndex
    );
    displayNameAndMode();


    // Actually activate the source
    dab->mute(true, true);
    dab->begin(0); // DAB Mode

    Preset previousPreset;
    if (currentMode == static_cast<int>(DABMode::LIST)) {
        previousPreset = listPresets[currentListIndex];
        targetPresetIndex = currentListIndex;
    } else if (currentMode == static_cast<int>(DABMode::MEMORY)) {
        previousPreset = memoryPresets[currentMemoryIndex];
        targetPresetIndex = currentMemoryIndex;
    }

    tunePreset(previousPreset);

    dab->speaker(SPEAKER_DIFF);
    dab->vol(35);
    dab->mute(false, false);

    active = true;
    LOG_INFO("Activated source \"%s\"", name);
}

uint8_t DABRadio::getCurrentModePresetIndex() const {
    return (currentMode == static_cast<int>(DABMode::LIST) ? currentListIndex : currentMemoryIndex);
}

std::vector<DABRadio::Preset> DABRadio::getCurrentModePresets() {
    return currentMode == static_cast<int>(DABMode::LIST) ? listPresets : memoryPresets;
}

DABRadio::Preset DABRadio::getCurrentPreset() {
    return getCurrentModePresets()[getCurrentModePresetIndex()];
}

bool DABRadio::isTuning() const {
    return getCurrentModePresetIndex() != targetPresetIndex;
}

void DABRadio::tick() {
    auto currentIndex = getCurrentModePresetIndex();
    if (currentIndex != targetPresetIndex && lastTargetPresetChange + CHANGE_PRESET_DELAY_MS < millis()) {
        if (currentMode == static_cast<int>(DABMode::LIST)) {
            currentListIndex = targetPresetIndex;
        } else if (currentMode == static_cast<int>(DABMode::MEMORY)) {
            currentMemoryIndex = targetPresetIndex;
        }

        const auto targetPreset = getCurrentModePresets()[targetPresetIndex];
        tunePreset(targetPreset);
    }
}

void DABRadio::tunePreset(Preset preset) {
    LOG_DEBUG("Tuning preset \"%s\"...", preset.name);
    char buffer[32];
    sprintf(buffer, ">>> %s...", preset.name);
    display->displayLine(buffer, 1);

    dab->tune(preset.dabEnsemble);
    dab->set_service(preset.serviceId);
    serviceInfo.clear();

    targetPresetIndex = currentMode == static_cast<int>(DABMode::LIST) ? currentListIndex : currentMemoryIndex;
    modeOrTuningChanged();

    LOG_INFO("Tuned preset \"%s\" (ensemble %d, service %d)", preset.name, preset.dabEnsemble, preset.serviceId);
}


void DABRadio::tuneLongPressed() {
    if (isTuning()) {
        // If we are tuning, we do not want to enter memorizing mode
        LOG_DEBUG("Tuning preset... Cannot enter memorizing mode.");
        return;
    }
    LOG_INFO("Entering memorizing mode...");
    memorizingPreset = true;
    displayServiceInfo();
}

void DABRadio::tuneReleased() {
    if (!memorizingPreset) {
        return;
    }

    memorizingPreset = false;

    auto currentPreset = getCurrentPreset();
    LOG_INFO("Saving preset \"%s\" to memory #%d", currentPreset.name, targetMemoryPreset);
    strcpy(memoryPresets[targetMemoryPreset].name, currentPreset.name);
    memoryPresets[targetMemoryPreset].dabEnsemble = currentPreset.dabEnsemble;
    memoryPresets[targetMemoryPreset].serviceId = currentPreset.serviceId;
    memoryPresets[targetMemoryPreset].compId = currentPreset.compId;

    targetMemoryPreset = 0;
    displayServiceInfo();
    savePresets();
}

/**
 * Changes the target preset while tuning through memory or list presets.
 * The preset is first targeted (the frequency does not change as it takes time) and when stable (no change in xxx ms)
 * the target preset is actually tuned in.
 * @param direction 'TUNE_UP' or 'TUNE_DOWN' (self-explanatory).
 */
void DABRadio::changeTargetPreset(const TuneDirection direction) {
    lastTargetPresetChange = millis();
    const std::vector<Preset> presets = getCurrentModePresets();

    if (targetPresetIndex == 0 && direction == TUNE_DOWN) {
        targetPresetIndex = presets.size() - 1;
    } else {
        targetPresetIndex = (targetPresetIndex + direction) % presets.size();
    }

    LOG_DEBUG("Targeting %s preset #%d-\"%s\"...",
              currentMode == static_cast<int>(DABMode::LIST) ? "list" : "memory",
              targetPresetIndex, presets[targetPresetIndex].name
    );
    displayServiceInfo();
}

/**
 * Changes the memory preset the current service will be writen to.
 * @param direction 'TUNE_UP' or 'TUNE_DOWN' (self-explanatory).
 */
void DABRadio::selectTargetMemoryPreset(const TuneDirection direction) {
    if (targetMemoryPreset == 0 && direction == TUNE_DOWN) {
        targetMemoryPreset = memoryPresets.size() - 1;
    } else {
        targetMemoryPreset = (targetMemoryPreset + direction) % memoryPresets.size();
    }
    LOG_DEBUG("Selected target memory preset %d", targetMemoryPreset);
    displayServiceInfo();
}


void DABRadio::tuneUp() {
    if (memorizingPreset) {
        selectTargetMemoryPreset(TUNE_UP);
    } else {
        changeTargetPreset(TUNE_UP);
    }
}

void DABRadio::tuneDown() {
    if (memorizingPreset) {
        selectTargetMemoryPreset(TUNE_DOWN);
    } else {
        changeTargetPreset(TUNE_DOWN);
    }
}

void DABRadio::modePressed() {
    currentMode = (currentMode + 1) % std::size(MODE_NAMES);
    displayNameAndMode();
    tunePreset(getCurrentPreset());
}

void DABRadio::modeDoublePressed() {
    const auto previousPreset = getCurrentModePresetIndex();
    refreshListPresets();
    if (currentMode == static_cast<int>(DABMode::LIST) && previousPreset >= listPresets.size()) {
        currentListIndex = listPresets.size() - 1;
    }
    tunePreset(getCurrentPreset());
}

void DABRadio::displayInformation() {
    if (targetPresetIndex != getCurrentModePresetIndex()) {
        LOG_DEBUG("Tuning mode skipping display of new information");
        return;
    }

    ServiceInfo newServiceInfo;
    newServiceInfo.copyFrom(serviceInfo);
    strncpy(newServiceInfo.serviceName, getCurrentPreset().name, 32);;

    if ((lastDabStatusRefresh + DAB_STATUS_REFRESH_DELAY) < millis()) {
        dab->status();
        newServiceInfo.bitRate = dab->bitrate;
        newServiceInfo.sampleRate = dab->samplerate;
        newServiceInfo.dabPlus = dab->dabplus;
        newServiceInfo.signalStrength = dab->signalstrength;
        newServiceInfo.snr = dab->snr;
        newServiceInfo.quality = dab->quality;
        lastDabStatusRefresh = millis();
    }

    strcpy(newServiceInfo.serviceData, dab->ServiceData);
    if (serviceInfo == newServiceInfo) {
        LOG_DEBUG("Skipping display of new information");
        return;
    }

    serviceInfo.copyFrom(newServiceInfo);
    LOG_INFO("Got new service information for \"%s\"", serviceInfo.serviceName);
    LOG_DEBUG(" > Data:        %s", serviceInfo.serviceData);
    LOG_DEBUG(" > Bitrate:     %dkHz", serviceInfo.bitRate);
    LOG_DEBUG(" > Sample rate: %dkHz", serviceInfo.sampleRate);
    LOG_DEBUG(" > Quality:     %d%%", serviceInfo.quality);
    LOG_DEBUG(" > DAB+:        %s", serviceInfo.dabPlus ? "true" : "false");

    displayServiceInfo();
}

void DABRadio::modeOrTuningChanged() {
    if (listPresets.empty()) {
        LOG_INFO("Preset list is empty... Triggering refresh.");
        refreshListPresets();
    }
    serviceInfo.clear();
    this->displayInformation();
    this->savePreferences();
}

/**
 * Display the service information in "standard" conditions (not tuning and not memorizing)
 */
void DABRadio::displayStandardServiceInfo() {
    char nameBuffer[32];
    char serviceTypeBuffer[5];
    char metricsBuffer[20];
    if (currentMode == static_cast<int>(DABMode::LIST)) {
        sprintf(nameBuffer, "#%02d %s", currentListIndex + 1, serviceInfo.serviceName);
    } else if (currentMode == static_cast<int>(DABMode::MEMORY)) {
        sprintf(nameBuffer, "M%02d %s", currentMemoryIndex + 1, serviceInfo.serviceName);
    }
    display->displayLine(nameBuffer, 1);

    if (strlen(serviceInfo.serviceData) > 0) {
        display->displayLine(serviceInfo.serviceData, 2, ROLLING_LEFT);
    } else {
        display->clearLine(2);
    }

    sprintf(serviceTypeBuffer, "%s", serviceInfo.dabPlus ? "DAB+" : "DAB");

    if (serviceInfo.bitRate > 0) {
        sprintf(metricsBuffer, "%dKHz Q:%d%%", serviceInfo.bitRate, serviceInfo.quality);
    } else {
        sprintf(metricsBuffer, "Q:%d%%", serviceInfo.quality);
    }
    display->displayJustified(serviceTypeBuffer, metricsBuffer, 3);
}

/**
 * Display service information while tuning to a preset.
 */
void DABRadio::displayTuningServiceInfo() {
    const auto presets = getCurrentModePresets();
    auto targetPreset = presets[targetPresetIndex];

    char buffer[32];
    sprintf(buffer, ">%02d %s", targetPresetIndex + 1, targetPreset.name);
    display->displayLine(buffer, 1);

    if (strlen(serviceInfo.serviceData) > 0 || serviceInfo.quality > 0) {
        serviceInfo.clear();
        display->clearLine(2);
        display->clearLine(3);
    }
}

void DABRadio::displayMemorizingServiceInfo() {
    char buffer[32];
    auto currentlyMemorizedPreset = memoryPresets[targetMemoryPreset];

    auto currentPreset = getCurrentPreset();

    sprintf(buffer, MEMORIZING_NAME, currentPreset.name);
    display->displayLine(buffer, 1);

    sprintf(buffer, MEMORY_PRESET_ID_AND_NAME, targetMemoryPreset + 1, currentlyMemorizedPreset.name);
    display->displayLine(buffer, 2);

    display->displayLine(RELEASE_TO_STORE, 3, CENTER);
}

void DABRadio::displayServiceInfo() {
    auto now = millis();
    LOG_DEBUG("Displaying service info...");
    if (memorizingPreset) {
        displayMemorizingServiceInfo();
    } else if (targetPresetIndex != getCurrentModePresetIndex()) {
        displayTuningServiceInfo();
    } else {
        displayStandardServiceInfo();
    }
    LOG_DEBUG("Displayed service info (%d ms)", millis() - now);
}

DABRadio::Preset DABRadio::getPresetFromJson(JsonObject preset) {
    auto newPreset = Preset{};
    strncpy(newPreset.name, preset[NAME_JSON_KEY], 32);
    newPreset.dabEnsemble = preset[ENSEMBLE_ID_JSON_KEY];
    newPreset.serviceId = preset[SERVICE_ID_JSON_KEY];
    newPreset.compId = preset[COMP_ID_JSON_KEY];
    return newPreset;
}

void DABRadio::loadPresetsFromJson(const String &jsonString) {
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
        listPresets.emplace_back(getPresetFromJson(preset));
    }

    // Loading memory presets
    memoryPresets.clear();
    const auto jsonMemoryPresets = doc[MEMORY_PRESETS_JSON_KEY].as<JsonArray>();

    for (JsonObject preset: jsonMemoryPresets) {
        memoryPresets.emplace_back(getPresetFromJson(preset));
    }

    LOG_INFO("Presets loaded: %d list presets, %d memory presets", listPresets.size(), memoryPresets.size());
}

void DABRadio::loadPresets() {
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

void DABRadio::addPresetToJsonArray(const JsonArray presetsArray, Preset preset) {
    auto jsonPreset = presetsArray.add<JsonObject>();
    jsonPreset[NAME_JSON_KEY] = preset.name;
    jsonPreset[ENSEMBLE_ID_JSON_KEY] = preset.dabEnsemble;
    jsonPreset[SERVICE_ID_JSON_KEY] = preset.serviceId;
    jsonPreset[COMP_ID_JSON_KEY] = preset.compId;
}

String DABRadio::presetsAsJson() const {
    JsonDocument doc;

    // Serialize the list presets
    const auto jsonListPresets = doc[LIST_PRESETS_JSON_KEY].to<JsonArray>();
    const auto jsonMemoryPresets = doc[MEMORY_PRESETS_JSON_KEY].to<JsonArray>();

    for (const auto preset: listPresets) {
        addPresetToJsonArray(jsonListPresets, preset);
    }

    for (const auto preset: memoryPresets) {
        addPresetToJsonArray(jsonMemoryPresets, preset);
    }

    String output;
    serializeJson(doc, output);

    return output;
}

void DABRadio::savePresets() const {
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

void DABRadio::savePreferences() {
    LOG_DEBUG("Saving preferences...");
    preferences.putInt(MODE_KEY, currentMode);
    if (currentMode == static_cast<int>(DABMode::LIST)) {
        preferences.putInt(LIST_PRESET_KEY, currentListIndex);
    } else if (currentMode == static_cast<int>(DABMode::MEMORY)) {
        preferences.putInt(MEMORY_PRESET_KEY, currentMemoryIndex);
    }
    LOG_INFO("Saved preferences (mode %s, LIST=%d, MEM=%d)", MODE_NAMES[currentMode], currentListIndex,
             currentMemoryIndex);
}

void DABRadio::displayNameAndMode() const {
    display->displayJustified(name, MODE_NAMES[currentMode], 0);
}


void DABRadio::refreshListPresets() {
    LOG_DEBUG("Refreshing list presets...");
    display->displayLine(REFRESHING_PRESETS, 1, CENTER);
    display->clearLine(2);
    display->clearLine(3);

    const uint8_t currentFrequencyIndex = dab->freq_index;
    dab->mute(true, true);
    char idAndNamBuffer[32];

    listPresets.clear();

    for (uint8_t frequencyIndex = 0; frequencyIndex < DAB_FREQS; frequencyIndex++) {
        const auto progress = static_cast<uint8_t>(round(
            100.0 * static_cast<double>(frequencyIndex) / DAB_FREQS));
        display->displayProgress(progress, 3);

        LOG_DEBUG("Looking into ensemble #%d - %.3fMHz (%d%%)...", frequencyIndex,
                  dab->freq_khz(frequencyIndex) / 1000.0,
                  progress);
        dab->tune(frequencyIndex);
        if (!dab->servicevalid()) {
            // Nothing here...
            continue;
        }

        LOG_DEBUG("Found ensemble at #%d. Saving services...", frequencyIndex);

        for (auto i = 0; i < dab->numberofservices; i++) {
            dab->status(dab->service[i].ServiceID, dab->service[i].CompID);
            if (dab->type == SERVICE_AUDIO) {
                // Save the preset
                auto newPreset = Preset{};
                newPreset.dabEnsemble = frequencyIndex;
                newPreset.serviceId = i;
                newPreset.compId = dab->service[i].CompID;
                strncpy(newPreset.name, dab->service[i].Label, 32);
                trim(newPreset.name);

                listPresets.emplace_back(newPreset);

                // Display what we have found and progress
                sprintf(idAndNamBuffer, PRESET_ID_AND_NAME, listPresets.size(), newPreset.name);
                display->displayLine(idAndNamBuffer, 2);

                LOG_DEBUG("New preset #%d - %d-%d %s (%d%%)", listPresets.size(), newPreset.dabEnsemble,
                          newPreset.serviceId, newPreset.name, progress);

                delay(100);
            }
        }
    }

    sort(listPresets.begin(), listPresets.end(), presetComparator);

    display->displayProgress(100, 3);

    // Resetting everything
    dab->tune(currentFrequencyIndex);
    dab->mute(false, false);
    delay(500);
    display->clearLine(1);
    display->clearLine(2);
    display->clearLine(3);
    displayServiceInfo();
    LOG_INFO("Refreshed list presets (found %d stations)... Saving.", listPresets.size());
    for (auto preset: listPresets) {
        LOG_DEBUG("  > %s @ %d - %d", preset.name, preset.dabEnsemble, preset.serviceId);
    }
    savePresets();
}


bool DABRadio::ServiceInfo::operator==(const ServiceInfo &other) const {
    if (this->frequencyIndex != other.frequencyIndex) return false;
    if (this->serviceId != other.serviceId) return false;
    if (this->compId != other.compId) return false;

    if (this->year != other.year) return false;
    if (this->month != other.month) return false;
    if (this->day != other.day) return false;
    if (this->hour != other.hour) return false;
    if (this->minute != other.minute) return false;

    if (this->bitRate != other.bitRate) return false;
    if (this->sampleRate != other.sampleRate) return false;
    if (this->dabPlus != other.dabPlus) return false;
    if (this->signalStrength != other.signalStrength) return false;
    if (this->snr != other.snr) return false;

    if (strcmp(this->serviceName, other.serviceName) != 0) return false;
    if (strcmp(this->serviceData, other.serviceData) != 0) return false;

    return true;
}

void DABRadio::ServiceInfo::copyFrom(const ServiceInfo &other) {
    this->frequencyIndex = other.frequencyIndex;
    this->serviceId = other.serviceId;
    this->compId = other.compId;

    this->year = other.year;
    this->month = other.month;
    this->day = other.day;
    this->hour = other.hour;
    this->minute = other.minute;

    this->bitRate = other.bitRate;
    this->sampleRate = other.sampleRate;
    this->quality = other.quality;
    this->dabPlus = other.dabPlus;
    this->signalStrength = other.signalStrength;
    this->snr = other.snr;

    strcpy(this->serviceName, other.serviceName);
    strcpy(this->serviceData, other.serviceData);
}

void DABRadio::ServiceInfo::clear() {
    this->frequencyIndex = 0;
    this->serviceId = 0;
    this->compId = 0;


    this->bitRate = 0;
    this->sampleRate = 0;
    this->dabPlus = false;
    this->signalStrength = 0;
    this->snr = 0;
    strcpy(this->serviceName, "");
    strcpy(this->serviceData, "");
}

