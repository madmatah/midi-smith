include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME splash
    SOURCES
        easing.test.cpp
        geometry.test.cpp
        pixel_canvas.test.cpp
        animation.test.cpp
)

target_compile_definitions(splash_unit_tests PRIVATE
    SPLASH_GOLDEN_DIRECTORY="${CMAKE_CURRENT_LIST_DIR}/golden")
