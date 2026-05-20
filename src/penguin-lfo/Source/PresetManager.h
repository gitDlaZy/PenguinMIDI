#pragma once
#include "LFOEngine.h"
#include <string>
#include <vector>
#include <array>

struct PresetData {
    std::string              name;
    std::array<LFOInstance, 4> lfos;
};

std::vector<PresetData> parsePresetsJson(const std::string& jsonStr);
std::vector<PresetData> loadPresetsFromFile(const std::string& filePath);
std::string             serializePreset(const PresetData& preset);
