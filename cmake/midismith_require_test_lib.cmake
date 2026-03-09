include_guard(GLOBAL)

function(midismith_require_test_lib LIB_NAME OUT_TARGET_NAME)
    if(NOT LIB_NAME MATCHES "^[a-z0-9]+(-[a-z0-9]+)*$")
        message(FATAL_ERROR
            "midismith_require_test_lib expects kebab-case monorepo lib name, got '${LIB_NAME}'")
    endif()

    get_filename_component(_MIDISMITH_TEST_REPO_ROOT
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.." ABSOLUTE)
    set(_MIDISMITH_TEST_LIB_CMAKELISTS
        "${_MIDISMITH_TEST_REPO_ROOT}/libs/${LIB_NAME}/CMakeLists.txt")

    if(NOT EXISTS "${_MIDISMITH_TEST_LIB_CMAKELISTS}")
        message(FATAL_ERROR
            "Test dependency '${LIB_NAME}' not found at '${_MIDISMITH_TEST_LIB_CMAKELISTS}'")
    endif()

    string(REPLACE "-" "_" _MIDISMITH_TEST_TARGET_NAME "${LIB_NAME}")
    if(NOT TARGET ${_MIDISMITH_TEST_TARGET_NAME})
        add_subdirectory(
            "${_MIDISMITH_TEST_REPO_ROOT}/libs/${LIB_NAME}"
            "libs/${LIB_NAME}"
        )
    endif()

    set(${OUT_TARGET_NAME} "${_MIDISMITH_TEST_TARGET_NAME}" PARENT_SCOPE)
endfunction()
