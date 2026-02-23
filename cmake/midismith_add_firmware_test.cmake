include_guard(GLOBAL)

set(_MIDISMITH_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")

macro(midismith_add_firmware_test)
    cmake_parse_arguments(_MSTEST "USE_FAKEIT" "NAME" "SOURCES;LINK_LIBRARIES;INCLUDE_DIRECTORIES" ${ARGN})

    include(${_MIDISMITH_CMAKE_DIR}/midismith_catch2.cmake)
    if(_MSTEST_USE_FAKEIT)
        include(${_MIDISMITH_CMAKE_DIR}/midismith_fakeit.cmake)
    endif()

    set(_MSTEST_TARGET "${_MSTEST_NAME}_unit_tests")

    add_executable(${_MSTEST_TARGET} ${_MSTEST_SOURCES})

    set(_MSTEST_LIBS Catch2::Catch2WithMain)
    if(_MSTEST_USE_FAKEIT)
        list(APPEND _MSTEST_LIBS fakeit)
    endif()
    if(_MSTEST_LINK_LIBRARIES)
        list(APPEND _MSTEST_LIBS ${_MSTEST_LINK_LIBRARIES})
    endif()
    target_link_libraries(${_MSTEST_TARGET} PRIVATE ${_MSTEST_LIBS})

    if(_MSTEST_INCLUDE_DIRECTORIES)
        target_include_directories(${_MSTEST_TARGET} PRIVATE ${_MSTEST_INCLUDE_DIRECTORIES})
    endif()

    target_compile_definitions(${_MSTEST_TARGET} PRIVATE UNIT_TESTS=1)
    target_compile_features(${_MSTEST_TARGET} PRIVATE cxx_std_20)

    if(PROJECT_IS_TOP_LEVEL)
        add_executable(unit_tests ALIAS ${_MSTEST_TARGET})
    endif()

    catch_discover_tests(${_MSTEST_TARGET})

    unset(_MSTEST_TARGET)
    unset(_MSTEST_LIBS)
    unset(_MSTEST_NAME)
    unset(_MSTEST_SOURCES)
    unset(_MSTEST_LINK_LIBRARIES)
    unset(_MSTEST_INCLUDE_DIRECTORIES)
    unset(_MSTEST_USE_FAKEIT)
    unset(_MSTEST_UNPARSED_ARGUMENTS)
    unset(_MSTEST_KEYWORDS_MISSING_VALUES)
endmacro()
