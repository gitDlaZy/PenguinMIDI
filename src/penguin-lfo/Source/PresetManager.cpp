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
    if (s == "Custom")        return LFOShape::Custom;
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
        case LFOShape::Custom:        return "Custom";
        default:                      return "Sine";
    }
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
            preset.name         = p["name"].get<std::string>();
            preset.filterLowCut  = p.value("filterLowCut",   20.0f);
            preset.filterHighCut = p.value("filterHighCut", 20000.0f);

            int i = 0;
            for (auto& l : p["lfos"]) {
                if (i >= 4) break;
                preset.lfos[i].shape      = shapeFromString(l["shape"]);
                preset.lfos[i].rateIndex  = rateIndexFromString(l["rate"]);
                preset.lfos[i].target     = targetFromString(l["target"]);
                preset.lfos[i].depth      = l["depth"].get<float>();
                preset.lfos[i].enabled    = l["enabled"].get<bool>();
                preset.lfos[i].smoothing   = l.value("smoothing",   0.0f);
                preset.lfos[i].pitchCenter = l.value("pitchCenter",  0.0f);

                if (l.contains("customWave")) {
                    auto& cw  = preset.lfos[i].customWave;
                    auto& jcw = l["customWave"];
                    cw.isStepMode = jcw.value("isStepMode", false);
                    cw.stepCount  = jcw.value("stepCount",  16);
                    if (jcw.contains("steps")) {
                        int n = 0;
                        for (auto& v : jcw["steps"]) {
                            if (n >= 32) break;
                            cw.steps[n++] = v.get<float>();
                        }
                    }
                    if (jcw.contains("nodes")) {
                        cw.nodeCount = 0;
                        for (auto& nd : jcw["nodes"]) {
                            if (cw.nodeCount >= 32) break;
                            cw.nodes[cw.nodeCount++] = {
                                nd["x"].get<float>(), nd["y"].get<float>()
                            };
                        }
                    }
                }
                ++i;
            }
            result.push_back(preset);
        }
    } catch (const json::exception&) {}
    return result;
}

std::vector<PresetData> loadPresetsFromFile(const std::string& filePath) {
    std::ifstream f(filePath);
    if (!f.is_open()) return {};
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    return parsePresetsJson(content);
}

bool savePresetsToFile(const std::vector<PresetData>& presets, const std::string& filePath) {
    json root;
    root["presets"] = json::array();
    for (const auto& p : presets)
        root["presets"].push_back(json::parse(serializePreset(p)));
    std::ofstream f(filePath);
    if (!f.is_open()) return false;
    f << root.dump(2);
    return f.good();
}

std::string serializePreset(const PresetData& preset) {
    json j;
    j["name"]          = preset.name;
    j["filterLowCut"]  = preset.filterLowCut;
    j["filterHighCut"] = preset.filterHighCut;
    j["lfos"]          = json::array();

    for (const auto& lfo : preset.lfos) {
        int rateIdx = std::clamp(lfo.rateIndex, 0, LFO_RATE_COUNT - 1);
        json lfoObj = {
            {"shape",       shapeToString(lfo.shape)},
            {"rate",        LFO_RATES[rateIdx].name},
            {"target",      targetToString(lfo.target)},
            {"depth",       lfo.depth},
            {"enabled",     lfo.enabled},
            {"smoothing",   lfo.smoothing},
            {"pitchCenter", lfo.pitchCenter}
        };

        if (lfo.shape == LFOShape::Custom) {
            json jcw;
            jcw["isStepMode"] = lfo.customWave.isStepMode;
            jcw["stepCount"]  = lfo.customWave.stepCount;
            json steps = json::array();
            for (int s = 0; s < lfo.customWave.stepCount; ++s)
                steps.push_back(lfo.customWave.steps[s]);
            jcw["steps"] = steps;
            json nodes = json::array();
            for (int n = 0; n < lfo.customWave.nodeCount; ++n)
                nodes.push_back({{"x", lfo.customWave.nodes[n].x},
                                 {"y", lfo.customWave.nodes[n].y}});
            jcw["nodes"]         = nodes;
            lfoObj["customWave"] = jcw;
        }
        j["lfos"].push_back(lfoObj);
    }
    return j.dump(2);
}
