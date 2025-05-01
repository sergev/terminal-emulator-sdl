To streamline the build process for the refactored SDL2-based terminal emulator and its Googletest-based unit tests, we’ll create a CMake script that builds both the main program and the tests. The script will use `FetchContent` to download and integrate Googletest, ensuring a self-contained build process without requiring manual installation of Googletest. The CMake configuration will:

- Organize the project into two targets: `terminal_emulator` (the main program) and `terminal_emulator_test` (the unit tests).
- Link against SDL2 and SDL2_ttf for the emulator and Googletest for the tests.
- Use `FetchContent` to fetch Googletest from its GitHub repository.
- Support both Linux and macOS, with appropriate include and library paths.
- Ensure the font paths in the code are valid or configurable.

### CMake Script
The `CMakeLists.txt` file below sets up the project, fetches Googletest, and defines the build targets. It assumes the source files are organized as previously provided: `terminal_emulator.h`, `terminal_emulator.cpp`, `main.cpp`, and `terminal_emulator_test.cpp`.

```x-cmake
cmake_minimum_required(VERSION 3.14)
project(TerminalEmulator VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2 and SDL2_ttf
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

# Use FetchContent to download Googletest
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Main executable
add_executable(terminal_emulator
    src/main.cpp
    src/terminal_emulator.cpp
)
target_include_directories(terminal_emulator PRIVATE src)
target_link_libraries(terminal_emulator PRIVATE SDL2::SDL2 SDL2_ttf::SDL2_ttf)

# Unit tests
add_executable(terminal_emulator_test
    src/terminal_emulator.cpp
    src/terminal_emulator_test.cpp
)
target_include_directories(terminal_emulator_test PRIVATE src)
target_link_libraries(terminal_emulator_test PRIVATE GTest::gtest_main)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(terminal_emulator_test)

# Installation
install(TARGETS terminal_emulator DESTINATION bin)
```

### Project Structure
The CMake script assumes the following directory structure:

```
project_root/
├── CMakeLists.txt
├── src/
│   ├── terminal_emulator.h
│   ├── terminal_emulator.cpp
│   ├── main.cpp
│   ├── terminal_emulator_test.cpp
```

Ensure the files from the previous response (`terminal_emulator.h`, `terminal_emulator.cpp`, `main.cpp`, `terminal_emulator_test.cpp`) are placed in the `src/` directory.

### Prerequisites
- **CMake**: Version 3.14 or higher.
- **SDL2 and SDL2_ttf**:
  - **Linux**: Install with `sudo apt-get install libsdl2-dev libsdl2-ttf-dev`.
  - **macOS**: Install with `brew install sdl2 sdl2_ttf`.
- **Font**: Ensure `/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf` (Linux) or `/System/Library/Fonts/Menlo.ttc` (macOS) exists, or modify the font path in `terminal_emulator.cpp` (e.g., via a CMake option or environment variable).
- **Compiler**: A C++17-compliant compiler (e.g., `g++`, `clang++`).
- **Internet**: Required for `FetchContent` to download Googletest during the first build.

### How to Build and Run
1. **Create a Build Directory**:
   ```bash
   mkdir build
   cd build
   ```

2. **Configure the Project**:
   ```bash
   cmake ..
   ```
   This downloads Googletest, finds SDL2 and SDL2_ttf, and generates build files. On macOS, if Homebrew is used, SDL2 might be in `/opt/homebrew`, so ensure CMake finds it (e.g., set `CMAKE_PREFIX_PATH`):
   ```bash
   cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew
   ```

3. **Build the Project**:
   ```bash
   cmake --build .
   ```
   This builds both `terminal_emulator` and `terminal_emulator_test`.

4. **Run the Emulator**:
   ```bash
   ./terminal_emulator
   ```
   This launches the terminal emulator, supporting `ESC c`, `ESC [ K`, per-character colors, Shift/Control modifiers, etc.

5. **Run the Unit Tests**:
   ```bash
   ctest
   ```
   Or directly:
   ```bash
   ./terminal_emulator_test
   ```
   Expected output:
   ```
   [==========] Running 7 tests from 1 test suite.
   [----------] 7 tests from TerminalEmulatorTest
   [ RUN      ] TerminalEmulatorTest.EscCResetsStateAndClearsScreen
   [       OK ] TerminalEmulatorTest.EscCResetsStateAndClearsScreen
   [ RUN      ] TerminalEmulatorTest.EscKClearsLine
   [       OK ] TerminalEmulatorTest.EscKClearsLine
   ...
   [  PASSED  ] 7 tests.
   ```

### Testing the Build
- **Emulator Functionality**:
  - Verify the emulator works as before:
    - Run `echo -e "\033c"` to reset and clear the screen.
    - Run `echo -e "\033[K"` to clear the line.
    - Test Shift+A (inputs 'A'), Ctrl+A (sends 0x01), and special keys (Tab, arrow keys).
    - Resize the window and check `stty size`.
    - Run `ls --color` for per-character colors.
- **Unit Tests**:
  - Ensure all 7 tests pass, covering `ESC c`, `ESC [ K`, colors, cursor movement, Shift/Control modifiers, and text buffer insertion.
  - Check for test failures and review console output for errors.

### Troubleshooting
- **SDL2/SDL2_ttf Not Found**:
  - Ensure libraries are installed.
  - On macOS, add `-DCMAKE_PREFIX_PATH=/opt/homebrew` or `/usr/local` to `cmake ..`.
  - On Linux, install `libsdl2-dev` and `libsdl2-ttf-dev`.
- **Font Path Issues**:
  - If the font fails to load, modify `terminal_emulator.cpp` to use a different font (e.g., `LiberationMono.ttf`) or add a CMake option:
    ```cmake
    target_compile_definitions(terminal_emulator PRIVATE FONT_PATH="/path/to/font.ttf")
    ```
    Then update `TTF_OpenFont` to use `FONT_PATH`.
- **Googletest Download Fails**:
  - Ensure internet access during `cmake ..`.
  - Alternatively, install Googletest locally and replace `FetchContent` with `find_package(GTest REQUIRED)`.
- **Linker Errors**:
  - Verify `-pthread` is included for Googletest (already in the script).
  - Check library paths for SDL2 and SDL2_ttf.

### CMake Script Notes
- **FetchContent**: Downloads Googletest 1.12.1, a stable release, ensuring consistent test builds.
- **Modularity**: Separates `terminal_emulator` and `terminal_emulator_test` targets, reusing `terminal_emulator.cpp`.
- **CTest Integration**: `gtest_discover_tests` enables `ctest` to run all Googletest cases.
- **Installation**: Installs the emulator to `bin/` (optional; run `make install`).
- **Cross-Platform**: Works on Linux and macOS with SDL2/SDL2_ttf installed.

### Additional Enhancements
If needed, the CMake script can be extended to:
- Add a configurable font path via CMake option.
- Include code coverage (`--coverage`, `lcov`).
- Support additional platforms (e.g., Windows with SDL2).
- Add mock libraries for SDL/PTY in tests.

If you encounter build issues, test failures, or want to extend the script (e.g., add coverage or custom font paths), please share the error output or requirements, and I’ll refine the solution!
