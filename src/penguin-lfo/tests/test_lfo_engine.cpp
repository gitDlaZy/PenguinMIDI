#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "LFOEngine.h"
using namespace Catch::Matchers;

TEST_CASE("Sine at phase 0 returns 0",    "[shapes]") {
    LFOInstance lfo; lfo.shape = LFOShape::Sine;
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.0f),  WithinAbs(0.0f,  0.001f));
}
TEST_CASE("Sine at phase 0.25 returns 1", "[shapes]") {
    LFOInstance lfo; lfo.shape = LFOShape::Sine;
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.25f), WithinAbs(1.0f,  0.001f));
}
TEST_CASE("Sine at phase 0.75 returns -1","[shapes]") {
    LFOInstance lfo; lfo.shape = LFOShape::Sine;
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.75f), WithinAbs(-1.0f, 0.001f));
}
TEST_CASE("Square below 0.5 returns 1",   "[shapes]") {
    LFOInstance lfo; lfo.shape = LFOShape::Square;
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.25f), WithinAbs( 1.0f, 0.001f));
}
TEST_CASE("Square at 0.5 returns -1",     "[shapes]") {
    LFOInstance lfo; lfo.shape = LFOShape::Square;
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.5f),  WithinAbs(-1.0f, 0.001f));
}
TEST_CASE("SawUp at phase 0 returns -1",  "[shapes]") {
    LFOInstance lfo; lfo.shape = LFOShape::SawUp;
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.0f),  WithinAbs(-1.0f, 0.001f));
}
TEST_CASE("SawDown at phase 0 returns 1", "[shapes]") {
    LFOInstance lfo; lfo.shape = LFOShape::SawDown;
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.0f),  WithinAbs( 1.0f, 0.001f));
}
TEST_CASE("Triangle at phase 0 returns -1",  "[shapes]") {
    LFOInstance lfo; lfo.shape = LFOShape::Triangle;
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.0f),  WithinAbs(-1.0f, 0.001f));
}
TEST_CASE("Triangle at phase 0.5 returns 1", "[shapes]") {
    LFOInstance lfo; lfo.shape = LFOShape::Triangle;
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.5f),  WithinAbs( 1.0f, 0.001f));
}
