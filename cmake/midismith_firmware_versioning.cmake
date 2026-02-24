include_guard(GLOBAL)

function(midismith_firmware_versioning TARGET)
    set(VERSION_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
    set(VERSION_GENERATED_HEADER "${VERSION_GENERATED_DIR}/app/version_build.hpp")
    set(VERSION_SCRIPT "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/generate_version_header.cmake")

    if(CMAKE_CONFIGURATION_TYPES)
        set(VERSION_BUILD_TYPE_ARG "$<CONFIG>")
    else()
        set(VERSION_BUILD_TYPE_ARG "${CMAKE_BUILD_TYPE}")
    endif()

    add_custom_command(
        OUTPUT "${VERSION_GENERATED_HEADER}" "FORCE_VERSION_CHECK"
        COMMAND "${CMAKE_COMMAND}"
                -DOUTPUT_HEADER=${VERSION_GENERATED_HEADER}
                -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
                -DVERSION_BUILD_TYPE=${VERSION_BUILD_TYPE_ARG}
                -P ${VERSION_SCRIPT}
        DEPENDS ${VERSION_SCRIPT}
        COMMENT "Checking for firmware version updates..."
        VERBATIM
    )

    add_custom_target(version_check ALL
        DEPENDS "${VERSION_GENERATED_HEADER}"
    )

    target_include_directories(${TARGET} PRIVATE ${VERSION_GENERATED_DIR})
    add_dependencies(${TARGET} version_check)
endfunction()
