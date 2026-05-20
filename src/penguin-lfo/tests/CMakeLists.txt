include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(PenguinLFO_Tests
    test_lfo_engine.cpp
    test_preset_manager.cpp
    ../Source/LFOEngine.cpp
)

target_link_libraries(PenguinLFO_Tests PRIVATE Catch2::Catch2WithMain)

target_include_directories(PenguinLFO_Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

include(CTest)
include(Catch)
catch_discover_tests(PenguinLFO_Tests)
