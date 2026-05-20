#include "PresetManager.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>

using json = nlohmann::json;

static LFOShape shapeFromString(const std::string& s) {
    if (s == "Sine")          return LFOShape::Sine;
    if (s == "Square")        return LFOShape::Square;
    if (s == "SawUp")         return LFOShape::SawUp;
    if (s == "SawDown")       return LFOShape::SawDown;
    if (s == "Triangle")      return LFOShape::Triangle;
    if (s == "SampleAndHold") return LFOShape::SampleAndHold;
    return LFOShape::Sine; // fallback for unknown/corrupt values
}

static std::string shapeToString(LFOShape s) {
    switch (s) {
        case LFOShape::Sine:          return "Sine";
        case LFOShape::Square:        return "Square";
        case LFOShape::SawUp:         return "SawUp";
        case LFOShape::SawDown:       return "SawDown";
        case LFOShape::Triangle:      return "Triangle";
        case LFOShape::SampleAndHold: return "SampleAndHold";
        default:                      return "Sine";
    }
}

static LFOTarget targetFromString(const std::string& s) {
    if (s == "Filter") return LFOTarget::Filter;
    if (s == "Pan")    return LFOTarget::Pan;
    if (s == "Pitch")  return LFOTarget::Pitch;
    return LFOTarget::Volume; // fallback for unknown/corrupt values
}

static std::string targetToString(LFOTarget t) {
    switch (t) {
        case LFOTarget::Volume: return "Volume";
        case LFOTarget::Filter: return "Filter";
        case LFOTarget::Pan:    return "Pan";
        case LFOTarget::Pitch:  return "Pitch";
        default:                return "Volume";
    }
}

static int rateIndexFromString(const std::string& s) {
    for (int i = 0; i < LFO_RATE_COUNT; ++i)
        if (LFO_RATES[i].name == s) return i;
    return LFO_RATE_1_4;
}

std::vector<PresetData> parsePresetsJson(const std::string& jsonStr) {
    std::vector<PresetData> result;
    try {
        auto j = json::parse(jsonStr);
        for (auto& p : j["presets"]) {
            PresetData preset;
            preset.name = p["name"].get<std::string>();
            int i = 0;
            for (auto& l : p["lfos"]) {
                if (i >= 4) break; // guard before write
                preset.lfos[i].shape     = shapeFromString(l["shape"]);
                preset.lfos[i].rateIndex = rateIndexFromString(l["rate"]);
                preset.lfos[i].target    = targetFromString(l["target"]);
                preset.lfos[i].depth     = l["depth"].get<float>();
                preset.lfos[i].enabled   = l["enabled"].get<bool>();
                ++i;
            }
            result.push_back(preset);
        }
    } catch (const json::exception&) {
        // malformed JSON — return empty rather than crashing the host
    }
    return result;
}

std::vector<PresetData> loadPresetsFromFile(const std::string& filePath) {
    std::ifstream f(filePath);
    if (!f.is_open()) return {};
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    return parsePresetsJson(content);
}

std::string serializePreset(const PresetData& preset) {
    json j;
    j["name"] = preset.name;
    j["lfos"] = json::array();
    for (const auto& lfo : preset.lfos) {
        int rateIdx = std::clamp(lfo.rateIndex, 0, LFO_RATE_COUNT - 1);
        j["lfos"].push_back({
            {"shape",   shapeToString(lfo.shape)},
            {"rate",    LFO_RATES[rateIdx].name},
            {"target",  targetToString(lfo.target)},
            {"depth",   lfo.depth},
            {"enabled", lfo.enabled}
        });
    }
    return j.dump(2);
}
