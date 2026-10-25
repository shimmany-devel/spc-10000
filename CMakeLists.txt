cmake_minimum_required(VERSION 3.20)
project(spc10000 VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -O2)
elseif (MSVC)
    add_compile_options(/W4 /O2 /EHsc /utf-8)
endif()

# Find SDL2 (Supports vcpkg, standard package, and config mode)
find_package(SDL2 REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
)

# Source files for SPC-1000 & SPC-10000
set(SOURCES
    # Core Z80
    src/core/z80/z80_cpu.cpp
    # Core Z8000
    src/core/z8000/z8000_cpu.cpp
    # Hardware peripherals
    src/hw/keyboard.cpp
    src/hw/vdg_mc6847.cpp
    src/hw/psg_ay8910.cpp
    src/hw/cassette.cpp
    # Base SPC-1000
    src/sys_spc1000/spc1000_board.cpp
    src/sys_spc1000/spc1000_machine.cpp
    # Evolution SPC-10000
    src/sys_spc10000/spc10000_board.cpp
    src/sys_spc10000/spc10000_machine.cpp
    # UI and entrypoint
    src/ui/emulator_window.cpp
    src/main.cpp
)

# Determine SDL2 link target / library
if (TARGET SDL2::SDL2)
    set(SPC_SDL2_LIB SDL2::SDL2)
    if (TARGET SDL2::SDL2main)
        list(APPEND SPC_SDL2_LIB SDL2::SDL2main)
    endif()
elseif (TARGET SDL2::SDL2-static)
    set(SPC_SDL2_LIB SDL2::SDL2-static)
else()
    set(SPC_SDL2_LIB ${SDL2_LIBRARIES})
endif()

add_executable(spc_launcher ${SOURCES})
target_link_libraries(spc_launcher PRIVATE ${SPC_SDL2_LIB})

# Test executable for snapshot and cassette verification
add_executable(test_snapshot_and_cassette
    tool/test_snapshot_and_cassette.cpp
    src/core/z80/z80_cpu.cpp
    src/hw/keyboard.cpp
    src/hw/vdg_mc6847.cpp
    src/hw/psg_ay8910.cpp
    src/hw/cassette.cpp
    src/sys_spc1000/spc1000_board.cpp
    src/sys_spc1000/spc1000_machine.cpp
)
target_link_libraries(test_snapshot_and_cassette PRIVATE ${SPC_SDL2_LIB})

# Tape load and run verification tool
add_executable(test_tape_load
    tool/debug_load.cpp
    src/core/z80/z80_cpu.cpp
    src/hw/keyboard.cpp
    src/hw/vdg_mc6847.cpp
    src/hw/psg_ay8910.cpp
    src/hw/cassette.cpp
    src/sys_spc1000/spc1000_board.cpp
    src/sys_spc1000/spc1000_machine.cpp
)
target_link_libraries(test_tape_load PRIVATE ${SPC_SDL2_LIB})

message(STATUS "Configured SPC-1000 / SPC-10000 Architecture")
