set(MIDISMITH_TARGET_FLAGS "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard")
get_filename_component(MIDISMITH_LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../STM32H7B0XX_FLASH.ld" ABSOLUTE)

include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/toolchain/starm-clang-base.cmake)
