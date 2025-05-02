cmake_minimum_required(VERSION 3.14)
project(TerminalEmulator VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Werror -Wshadow)

# Find SDL2 and SDL2_ttf
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(ICU REQUIRED COMPONENTS uc)

# Use FetchContent to download Googletest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    EXCLUDE_FROM_ALL
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Main executable
add_executable(terminal_emulator
    src/main.cpp
    src/sdl_interface.cpp
    src/terminal_logic.cpp
)
target_include_directories(terminal_emulator PRIVATE
    ${SDL2_INCLUDE_DIRS}
    SDL2_ttf::SDL2_ttf
    ${gtest_SOURCE_DIR}/include
)
target_link_libraries(terminal_emulator PRIVATE
    SDL2::SDL2
    SDL2_ttf::SDL2_ttf
    ICU::uc
)

# Unit tests
add_executable(unit_tests
    src/terminal_logic.cpp
    src/unit_tests.cpp
)
target_include_directories(unit_tests PRIVATE src)
target_link_libraries(unit_tests PRIVATE
    GTest::gtest_main
    ICU::uc
)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(unit_tests)

# Installation
install(TARGETS terminal_emulator DESTINATION bin)
