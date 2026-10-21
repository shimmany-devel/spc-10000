cmake_minimum_required(VERSION 3.20)
project(spc10000 VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -O2)
endif()

# Find SDL2
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

add_executable(spc_launcher ${SOURCES})
target_link_libraries(spc_launcher PRIVATE ${SDL2_LIBRARIES})

message(STATUS "Configured SPC-1000 / SPC-10000 Architecture")
