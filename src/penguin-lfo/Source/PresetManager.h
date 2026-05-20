#pragma once
#include "LFOEngine.h"
#include <string>
#include <vector>
#include <array>

struct PresetData {
    std::string                name;
    std::array<LFOInstance, 4> lfos;
    float filterLowCut  = 20.0f;
    float filterHighCut = 20000.0f;
};

std::vector<PresetData> parsePresetsJson(const std::string& jsonStr);
std::vector<PresetData> loadPresetsFromFile(const std::string& filePath);
std::string             serializePreset(const PresetData& preset);
bool                    savePresetsToFile(const std::vector<PresetData>& presets,
                                          const std::string& filePath);
