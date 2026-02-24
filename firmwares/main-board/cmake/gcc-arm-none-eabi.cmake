set(MIDISMITH_TARGET_FLAGS "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard")
set(MIDISMITH_RELEASE_C_FLAGS "-Os -g0")
set(MIDISMITH_RELEASE_CXX_FLAGS "-Os -g0")
get_filename_component(MIDISMITH_LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../STM32H7B0XX_FLASH.ld" ABSOLUTE)

include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/toolchain/gcc-arm-none-eabi-base.cmake)
