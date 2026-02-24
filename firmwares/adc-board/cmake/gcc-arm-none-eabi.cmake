set(MIDISMITH_TARGET_FLAGS "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard")

option(FORCE_DEBUG_SYMBOLS "Force debug symbols even in Release mode" OFF)
if(FORCE_DEBUG_SYMBOLS)
    set(RELEASE_DEBUG_FLAGS "-g3")
else()
    set(RELEASE_DEBUG_FLAGS "-g0")
endif()

set(MIDISMITH_RELEASE_C_FLAGS "-O3 -ffast-math ${RELEASE_DEBUG_FLAGS}")
set(MIDISMITH_RELEASE_CXX_FLAGS "-O3 -ffast-math ${RELEASE_DEBUG_FLAGS}")
get_filename_component(MIDISMITH_LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../STM32H743XX_FLASH.ld" ABSOLUTE)

include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/toolchain/gcc-arm-none-eabi-base.cmake)
