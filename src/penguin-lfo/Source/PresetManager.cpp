#include "PresetManager.h"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

static LFOShape shapeFromString(const std::string& s) {
    if (s == "Sine")          return LFOShape::Sine;
    if (s == "Square")        return LFOShape::Square;
    if (s == "SawUp")         return LFOShape::SawUp;
    if (s == "SawDown")       return LFOShape::SawDown;
    if (s == "Triangle")      return LFOShape::Triangle;
    if (s == "SampleAndHold") return LFOShape::SampleAndHold;
    return LFOShape::Sine;
}

static std::string shapeToString(LFOShape s) {
    switch (s) {
        case LFOShape::Sine:          return "Sine";
        case LFOShape::Square:        return "Square";
        case LFOShape::SawUp:         return "SawUp";
        case LFOShape::SawDown:       return "SawDown";
        case LFOShape::Triangle:      return "Triangle";
        case LFOShape::SampleAndHold: return "SampleAndHold";
    }
    return "Sine";
}

static LFOTarget targetFromString(const std::string& s) {
    if (s == "Filter") return LFOTarget::Filter;
    if (s == "Pan")    return LFOTarget::Pan;
    if (s == "Pitch")  return LFOTarget::Pitch;
    return LFOTarget::Volume;
}

static std::string targetToString(LFOTarget t) {
    switch (t) {
        case LFOTarget::Volume: return "Volume";
        case LFOTarget::Filter: return "Filter";
        case LFOTarget::Pan:    return "Pan";
        case LFOTarget::Pitch:  return "Pitch";
    }
    return "Volume";
}

static int rateIndexFromString(const std::string& s) {
    for (int i = 0; i < LFO_RATE_COUNT; ++i)
        if (LFO_RATES[i].name == s) return i;
    return LFO_RATE_1_4;
}

std::vector<PresetData> parsePresetsJson(const std::string& jsonStr) {
    std::vector<PresetData> result;
    auto j = json::parse(jsonStr);
    for (auto& p : j["presets"]) {
        PresetData preset;
        preset.name = p["name"].get<std::string>();
        int i = 0;
        for (auto& l : p["lfos"]) {
            preset.lfos[i].shape     = shapeFromString(l["shape"]);
            preset.lfos[i].rateIndex = rateIndexFromString(l["rate"]);
            preset.lfos[i].target    = targetFromString(l["target"]);
            preset.lfos[i].depth     = l["depth"].get<float>();
            preset.lfos[i].enabled   = l["enabled"].get<bool>();
            if (++i >= 4) break;
        }
        result.push_back(preset);
    }
    return result;
}

std::vector<PresetData> loadPresetsFromFile(const std::string& filePath) {
    std::ifstream f(filePath);
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    return parsePresetsJson(content);
}

std::string serializePreset(const PresetData& preset) {
    json j;
    j["name"] = preset.name;
    j["lfos"] = json::array();
    for (const auto& lfo : preset.lfos) {
        j["lfos"].push_back({
            {"shape",   shapeToString(lfo.shape)},
            {"rate",    LFO_RATES[lfo.rateIndex].name},
            {"target",  targetToString(lfo.target)},
            {"depth",   lfo.depth},
            {"enabled", lfo.enabled}
        });
    }
    return j.dump(2);
}
