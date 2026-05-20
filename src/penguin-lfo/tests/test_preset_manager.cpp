#include <catch2/catch_test_macros.hpp>
#include "PresetManager.h"

TEST_CASE("Parse a single preset from JSON", "[presets]") {
    const char* json = R"({
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
    auto presets = parsePresetsJson(json);
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
}
