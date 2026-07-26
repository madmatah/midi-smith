cmake_minimum_required(VERSION 3.22)
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_firmware_common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_require_lib.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_flash_layout.cmake)
set(BOOT_FIRMWARE_TARGET "bootloader")
set(BOOT_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}")

project(${BOOT_FIRMWARE_TARGET})
message("Build type: " ${CMAKE_BUILD_TYPE})

midismith_require_lib(boot-control)
midismith_require_lib(byte-codec)
midismith_require_lib(checksum)
midismith_require_lib(firmware-image)
midismith_require_lib(firmware-installer)
midismith_require_lib(flash-layout)
midismith_require_lib(product-id)

if(HOST_TESTS)
    # Everything this package decides lives in libs/boot-control and libs/firmware-image, which
    # own the host tests. What is left here is the bare-metal shell that cannot run on a host.
    return()
endif()

add_executable(${BOOT_FIRMWARE_TARGET})

midismith_add_stm32cubemx(${BOOT_FIRMWARE_TARGET})

target_sources(${BOOT_FIRMWARE_TARGET} PRIVATE
    app/src/boot_entry.cpp
    app/src/application.cpp
    bsp/src/application_launcher.cpp
    bsp/src/application_slot_flash.cpp
    bsp/src/internal_flash.cpp
    bsp/src/journal_storage.cpp
    bsp/src/status_led.cpp
)

target_include_directories(${BOOT_FIRMWARE_TARGET} PRIVATE
    app/include
    bsp/include
)

midismith_firmware_link_map(${BOOT_FIRMWARE_TARGET})

target_link_libraries(${BOOT_FIRMWARE_TARGET}
    stm32cubemx
    boot_control
    byte_codec
    checksum
    firmware_image
    firmware_installer
    flash_layout
    product_id
)

midismith_check_flash_layout(${BOOT_FIRMWARE_TARGET} SLOT BOOTLOADER)
