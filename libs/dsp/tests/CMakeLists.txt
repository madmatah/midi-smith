include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(dsp_unit_tests
    converters/linear_scaler.test.cpp
    converters/tia_current_converter.test.cpp
    engine/decimated.test.cpp
    engine/observers.test.cpp
    engine/temporal_continuity_guard.test.cpp
    engine/workflow.test.cpp
    filters/biquad.test.cpp
    filters/constant_filter.test.cpp
    filters/ema_filter.test.cpp
    filters/identity_filter.test.cpp
    filters/sg5_smoother.test.cpp
    filters/simple_moving_average.test.cpp
    logic/and.test.cpp
    logic/gate_open.test.cpp
    logic/input_gate.test.cpp
    logic/is_false.test.cpp
    logic/is_true.test.cpp
    logic/or.test.cpp
    logic/switch.test.cpp
    math/backward_difference.test.cpp
    math/central_difference.test.cpp
    math/sliding_linear_regression.test.cpp
)
target_link_libraries(dsp_unit_tests PRIVATE Catch2::Catch2WithMain dsp)
target_compile_definitions(dsp_unit_tests PRIVATE UNIT_TESTS=1)
target_compile_features(dsp_unit_tests PRIVATE cxx_std_20)

if(DEFINED catch2_SOURCE_DIR)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()
include(Catch)
catch_discover_tests(dsp_unit_tests)
