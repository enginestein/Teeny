cmake_minimum_required(VERSION 3.13...3.27)

# The hardware target is a SparkFun Thing Plus RP2040 (shortened to "Thing
# Plus" in this codebase). Its Pico-SDK board header carries the microSD slot
# wiring, 16 MB flash size and boot stage-2 settings.
#
# This must be set BEFORE including pico_sdk_import.cmake: the import script
# pulls in pico_sdk_init.cmake, which locks in PICO_BOARD via the platform
# pre-load (defaulting to "pico" if not already defined here).
set(PICO_BOARD sparkfun_thingplus)

include(pico_sdk_import.cmake)

project(Teeny C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

add_executable(teeny
    firmware/main.c
    firmware/boot/boot.c
    firmware/kernel/kernel.c
    firmware/hardware/usb/usb_protocol.c
    firmware/hardware/usb/usb_link.c
    firmware/hardware/usb/usb_descriptors.c
    firmware/hardware/sd/sd_spi.c
    firmware/hardware/video/video_usb.c
    firmware/hardware/display/oled.c
    firmware/hardware/input/joystick.c
    firmware/hardware/mode.c
    firmware/graphics/display.c
    firmware/graphics/phase1_pattern.c
    firmware/input/input.c
    firmware/games/snake.c
    firmware/games/breakout.c
    firmware/games/invaders.c
    firmware/games/pong.c
    firmware/games/tetris.c
    firmware/games/runner.c
    firmware/games/pacmate.c
    firmware/games/flappy.c
    firmware/games/launcher.c
    firmware/system/console.c
    firmware/storage/storage.c
    firmware/system/fatfs/diskio.c
    firmware/system/fatfs/ff.c
)

target_include_directories(teeny PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/firmware/include
    ${CMAKE_CURRENT_LIST_DIR}/firmware/hardware/usb
    ${CMAKE_CURRENT_LIST_DIR}/firmware/system/fatfs
)

target_link_libraries(teeny
    pico_stdlib
    hardware_spi
    hardware_i2c
    tinyusb_device
    tinyusb_board
    tinyusb_additions
)

target_compile_options(teeny PRIVATE -Wall -Wextra)

pico_enable_stdio_uart(teeny 0)
pico_enable_stdio_usb(teeny 0)

pico_add_extra_outputs(teeny)