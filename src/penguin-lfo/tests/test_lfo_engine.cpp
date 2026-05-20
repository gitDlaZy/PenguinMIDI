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
TEST_CASE("lfoAdvance wraps phase back to [0,1)", "[advance]") {
    LFOInstance lfo; lfo.shape = LFOShape::Sine;
    float inc = 0.6f;
    lfoAdvance(lfo, inc); // phase = 0.6
    lfoAdvance(lfo, inc); // phase would be 1.2 → wraps to 0.2
    REQUIRE(lfo.phase >= 0.0f);
    REQUIRE(lfo.phase < 1.0f);
    REQUIRE_THAT(lfo.phase, WithinAbs(0.2f, 0.001f));
}
TEST_CASE("lfoAdvance S&H value stays in [-1, 1] after wrap", "[advance]") {
    LFOInstance lfo; lfo.shape = LFOShape::SampleAndHold;
    for (int i = 0; i < 20; ++i)
        lfoAdvance(lfo, 0.6f); // force multiple wraps
    REQUIRE(lfo.sampleAndHoldValue >= -1.0f);
    REQUIRE(lfo.sampleAndHoldValue <= 1.0f);
}
