include_guard(GLOBAL)

macro(midismith_require_lib LIB_NAME)
    string(REPLACE "-" "_" _TARGET_NAME "${LIB_NAME}")
    if(NOT TARGET ${_TARGET_NAME})
        add_subdirectory(
            "${CMAKE_CURRENT_LIST_DIR}/../../libs/${LIB_NAME}"
            "libs/${LIB_NAME}"
        )
    endif()
    unset(_TARGET_NAME)
endmacro()
