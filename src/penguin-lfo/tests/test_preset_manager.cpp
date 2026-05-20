#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <fstream>
#include "PresetManager.h"
using namespace Catch::Matchers;

TEST_CASE("Parse a single preset from JSON", "[presets]") {
    const char* jsonStr = R"({
      "presets": [{
        "name": "Test",
        "lfos": [
          {"shape":"Sine",    "rate":"1/4",  "target":"Volume","depth":0.8,"enabled":true},
          {"shape":"Square",  "rate":"1/8",  "target":"Filter","depth":0.5,"enabled":true},
          {"shape":"SawUp",   "rate":"1/16", "target":"Pan",   "depth":0.3,"enabled":false},
          {"shape":"Triangle","rate":"1/32", "target":"Pitch", "depth":0.2,"enabled":false}
        ]
      }]
    })";
    auto presets = parsePresetsJson(jsonStr);
    REQUIRE(presets.size() == 1);
    REQUIRE(presets[0].name == "Test");
    REQUIRE(presets[0].lfos[0].shape     == LFOShape::Sine);
    REQUIRE(presets[0].lfos[0].rateIndex == LFO_RATE_1_4);
    REQUIRE(presets[0].lfos[0].target    == LFOTarget::Volume);
    REQUIRE(presets[0].lfos[0].depth     == 0.8f);
    REQUIRE(presets[0].lfos[0].enabled   == true);
    REQUIRE(presets[0].lfos[1].shape     == LFOShape::Square);
    REQUIRE(presets[0].lfos[2].enabled   == false);
}

TEST_CASE("Serialize and re-parse round-trips correctly", "[presets]") {
    PresetData original;
    original.name                = "Round Trip";
    original.lfos[0].shape       = LFOShape::Square;
    original.lfos[0].rateIndex   = LFO_RATE_1_8T;
    original.lfos[0].target      = LFOTarget::Filter;
    original.lfos[0].depth       = 0.6f;
    original.lfos[0].enabled     = true;
    original.lfos[1].enabled     = false;
    original.lfos[2].enabled     = false;
    original.lfos[3].enabled     = false;

    std::string wrapped = "{\"presets\":[" + serializePreset(original) + "]}";
    auto loaded = parsePresetsJson(wrapped);

    REQUIRE(loaded[0].name                == "Round Trip");
    REQUIRE(loaded[0].lfos[0].shape       == LFOShape::Square);
    REQUIRE(loaded[0].lfos[0].rateIndex   == LFO_RATE_1_8T);
    REQUIRE(loaded[0].lfos[0].target      == LFOTarget::Filter);
    REQUIRE(loaded[0].lfos[1].enabled     == false);
    REQUIRE(loaded[0].lfos[2].enabled     == false);
    REQUIRE(loaded[0].lfos[3].enabled     == false);
}

TEST_CASE("Malformed JSON returns empty preset list", "[presets]") {
    REQUIRE(parsePresetsJson("").empty());
    REQUIRE(parsePresetsJson("{invalid json}").empty());
    REQUIRE(parsePresetsJson("{\"presets\": null}").empty());
}

TEST_CASE("Factory presets load from file", "[presets]") {
    std::ifstream f("Presets/presets.json");
    REQUIRE(f.is_open());
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    auto presets = parsePresetsJson(content);
    REQUIRE(presets.size() >= 7);
    for (const auto& p : presets)
        REQUIRE(!p.name.empty());
    REQUIRE(presets[0].lfos[0].enabled   == true);
    REQUIRE(presets[0].lfos[0].depth     > 0.0f);
    REQUIRE(presets[0].lfos[0].rateIndex != LFO_RATE_1_4);
}

TEST_CASE("Preset round-trips smoothing and pitchCenter", "[presets]") {
    PresetData p;
    p.name = "Smooth Test";
    p.lfos[0].smoothing   = 0.5f;
    p.lfos[0].pitchCenter = -0.3f;
    p.lfos[0].enabled     = true;
    p.lfos[1].enabled = p.lfos[2].enabled = p.lfos[3].enabled = false;

    std::string wrapped = "{\"presets\":[" + serializePreset(p) + "]}";
    auto loaded = parsePresetsJson(wrapped);

    REQUIRE_THAT(loaded[0].lfos[0].smoothing,   WithinAbs(0.5f,  0.001f));
    REQUIRE_THAT(loaded[0].lfos[0].pitchCenter, WithinAbs(-0.3f, 0.001f));
}

TEST_CASE("Preset round-trips filter range", "[presets]") {
    PresetData p;
    p.name          = "Filter Test";
    p.filterLowCut  = 200.0f;
    p.filterHighCut = 8000.0f;
    p.lfos[0].enabled = p.lfos[1].enabled = p.lfos[2].enabled = p.lfos[3].enabled = false;

    std::string wrapped = "{\"presets\":[" + serializePreset(p) + "]}";
    auto loaded = parsePresetsJson(wrapped);

    REQUIRE_THAT(loaded[0].filterLowCut,  WithinAbs(200.0f,  0.1f));
    REQUIRE_THAT(loaded[0].filterHighCut, WithinAbs(8000.0f, 0.1f));
}

TEST_CASE("Old preset without new fields loads with defaults", "[presets]") {
    const char* oldJson = R"({
      "presets": [{
        "name": "Old Preset",
        "lfos": [
          {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.5,"enabled":true},
          {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false},
          {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false},
          {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false}
        ]
      }]
    })";
    auto presets = parsePresetsJson(oldJson);
    REQUIRE(presets.size() == 1);
    REQUIRE_THAT(presets[0].lfos[0].smoothing,   WithinAbs(0.0f,     0.001f));
    REQUIRE_THAT(presets[0].lfos[0].pitchCenter, WithinAbs(0.0f,     0.001f));
    REQUIRE_THAT(presets[0].filterLowCut,        WithinAbs(20.0f,    0.1f));
    REQUIRE_THAT(presets[0].filterHighCut,       WithinAbs(20000.0f, 1.0f));
}

TEST_CASE("Preset round-trips custom waveform (step mode)", "[presets]") {
    PresetData p;
    p.name = "Step Test";
    p.lfos[0].shape                 = LFOShape::Custom;
    p.lfos[0].customWave.isStepMode = true;
    p.lfos[0].customWave.stepCount  = 4;
    p.lfos[0].customWave.steps[0]   = -1.0f;
    p.lfos[0].customWave.steps[1]   =  0.5f;
    p.lfos[0].customWave.steps[2]   =  0.0f;
    p.lfos[0].customWave.steps[3]   =  1.0f;
    p.lfos[0].enabled = true;
    p.lfos[1].enabled = p.lfos[2].enabled = p.lfos[3].enabled = false;

    std::string wrapped = "{\"presets\":[" + serializePreset(p) + "]}";
    auto loaded = parsePresetsJson(wrapped);

    REQUIRE(loaded[0].lfos[0].shape                 == LFOShape::Custom);
    REQUIRE(loaded[0].lfos[0].customWave.isStepMode == true);
    REQUIRE(loaded[0].lfos[0].customWave.stepCount  == 4);
    REQUIRE_THAT(loaded[0].lfos[0].customWave.steps[0], WithinAbs(-1.0f, 0.001f));
    REQUIRE_THAT(loaded[0].lfos[0].customWave.steps[3], WithinAbs( 1.0f, 0.001f));
}

TEST_CASE("Preset round-trips custom waveform (breakpoint mode)", "[presets]") {
    PresetData p;
    p.name = "Node Test";
    p.lfos[0].shape                 = LFOShape::Custom;
    p.lfos[0].customWave.isStepMode = false;
    p.lfos[0].customWave.nodeCount  = 3;
    p.lfos[0].customWave.nodes[0]   = {0.0f, -1.0f};
    p.lfos[0].customWave.nodes[1]   = {0.5f,  1.0f};
    p.lfos[0].customWave.nodes[2]   = {1.0f, -1.0f};
    p.lfos[0].enabled = true;
    p.lfos[1].enabled = p.lfos[2].enabled = p.lfos[3].enabled = false;

    std::string wrapped = "{\"presets\":[" + serializePreset(p) + "]}";
    auto loaded = parsePresetsJson(wrapped);

    REQUIRE(loaded[0].lfos[0].shape                 == LFOShape::Custom);
    REQUIRE(loaded[0].lfos[0].customWave.isStepMode == false);
    REQUIRE(loaded[0].lfos[0].customWave.nodeCount  == 3);
    REQUIRE_THAT(loaded[0].lfos[0].customWave.nodes[1].x, WithinAbs(0.5f, 0.001f));
    REQUIRE_THAT(loaded[0].lfos[0].customWave.nodes[1].y, WithinAbs(1.0f, 0.001f));
}
