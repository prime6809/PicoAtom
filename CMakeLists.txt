cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

include(pico_extras_import.cmake)

set (projname "PicoAtom")
project(${projname} C CXX ASM)

pico_sdk_init()

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter" )

#
# Pi Pico
#
include_directories(. ILI9341 Atom AtomMMC PS2 Utils)

add_executable(${projname}
  main.c
  Atom.c
  fake6502.c
  6847.c
  AtomMMC/AtoMMCInterface.c
  AtomMMC/atmmc2core.c
  AtomMMC/atmmc2wfn.c
  AtomMMC/ff.c
  AtomMMC/diskio.c
  AtomMMC/mmcio.c
  AtomMMC/wildcard.c
  ILI9341/ILI9341_Driver.c
  ILI9341/ILI9341_DMA.c
  ILI9341/ILI9341_Dragon.c
  ILI9341/ILI9341_GFX.c
  ILI9341/ILI9341_Text.c
  PS2/matrix_kbd.c
  PS2/ps2kbd.c
  Utils/delays.c
  Utils/status.c
  Utils/hexout.c
  Utils/str_util.c
  )

target_compile_definitions(${projname} PUBLIC -DSERIAL_STATUS )

target_link_libraries(${projname} PRIVATE
  pico_multicore
  pico_stdlib
  hardware_pio
  hardware_spi
  hardware_dma
  )

pico_add_extra_outputs(${projname})

pico_enable_stdio_uart(${projname} 1)

