include_guard(GLOBAL)
include(${CMAKE_CURRENT_LIST_DIR}/midismith_require_lib.cmake)

macro(midismith_add_infra_lib)
    cmake_parse_arguments(_MSLIB "" "NAME" "SOURCES;DEPENDS" ${ARGN})

    add_library(${_MSLIB_NAME} INTERFACE)
    target_compile_features(${_MSLIB_NAME} INTERFACE cxx_std_20)
    target_include_directories(${_MSLIB_NAME} INTERFACE include)

    # Resolve midismith dependencies.
    if(_MSLIB_DEPENDS)
        set(_MSLIB_INFRA_LINK_LIBS "")
        foreach(_dep ${_MSLIB_DEPENDS})
            midismith_require_lib(${_dep})
            string(REPLACE "-" "_" _dep_target "${_dep}")
            list(APPEND _MSLIB_INFRA_LINK_LIBS ${_dep_target})
        endforeach()
        target_link_libraries(${_MSLIB_NAME} INTERFACE ${_MSLIB_INFRA_LINK_LIBS})
        unset(_dep)
        unset(_dep_target)
        unset(_MSLIB_INFRA_LINK_LIBS)
    endif()

    # Sources are only compiled in firmware builds (not in host tests).
    if(NOT HOST_TESTS AND _MSLIB_SOURCES)
        target_sources(${_MSLIB_NAME} INTERFACE ${_MSLIB_SOURCES})
    endif()

    unset(_MSLIB_NAME)
    unset(_MSLIB_SOURCES)
    unset(_MSLIB_DEPENDS)
    unset(_MSLIB_UNPARSED_ARGUMENTS)
    unset(_MSLIB_KEYWORDS_MISSING_VALUES)
endmacro()
