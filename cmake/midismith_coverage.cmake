include_guard(GLOBAL)

function(midismith_enable_coverage_compile TARGET_NAME)
    if(NOT HOST_TESTS)
        return()
    endif()

    get_target_property(_TARGET_TYPE ${TARGET_NAME} TYPE)
    if(_TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    target_compile_options(${TARGET_NAME} PRIVATE -O0 -g --coverage)

    unset(_TARGET_TYPE)
endfunction()

function(midismith_enable_coverage_link TARGET_NAME)
    if(NOT HOST_TESTS)
        return()
    endif()

    get_target_property(_TARGET_TYPE ${TARGET_NAME} TYPE)
    if(_TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    target_link_options(${TARGET_NAME} PRIVATE --coverage)

    unset(_TARGET_TYPE)
endfunction()

function(midismith_register_test_executable TARGET_NAME)
    if(NOT HOST_TESTS)
        return()
    endif()

    get_property(_REGISTERED_TEST_TARGETS GLOBAL PROPERTY MIDISMITH_TEST_EXECUTABLE_TARGETS)
    if(NOT _REGISTERED_TEST_TARGETS)
        set(_REGISTERED_TEST_TARGETS "")
    endif()

    list(APPEND _REGISTERED_TEST_TARGETS ${TARGET_NAME})
    list(REMOVE_DUPLICATES _REGISTERED_TEST_TARGETS)
    set_property(GLOBAL PROPERTY MIDISMITH_TEST_EXECUTABLE_TARGETS "${_REGISTERED_TEST_TARGETS}")

    unset(_REGISTERED_TEST_TARGETS)
endfunction()
