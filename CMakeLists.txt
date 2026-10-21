cmake_minimum_required(VERSION 3.20)
project(spc10000 VERSION 0.2.0 LANGUAGES C CXX)

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

# Source files
set(SOURCES
    src/core/z80/z80_cpu.cpp
    src/hw/keyboard.cpp
    src/hw/vdg_mc6847.cpp
    src/hw/psg_ay8910.cpp
    src/hw/cassette.cpp
    src/sys_spc1000/spc1000_board.cpp
    src/sys_spc1000/spc1000_machine.cpp
    src/ui/emulator_window.cpp
    src/main.cpp
)

# Emulator executable
add_executable(spc_launcher ${SOURCES})
target_link_libraries(spc_launcher PRIVATE ${SDL2_LIBRARIES})

message(STATUS "Configured SPC-1000 Emulator (C++20 & SDL2)")
