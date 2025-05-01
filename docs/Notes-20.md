The error occurs because the unit tests in `terminal_emulator_test.cpp` attempt to access private member variables (e.g., `current_attr`, `text_buffer`, `cursor`, `dirty_lines`, `texture_cache`) of the `TerminalEmulator` class, which are not accessible outside the class. To resolve this, we need to make these members accessible to the tests while maintaining encapsulation for the main program. There are several approaches to achieve this:

1. **Friend Class**: Declare the test fixture as a `friend` of `TerminalEmulator`, allowing it to access private members.
2. **Protected Members**: Make the members `protected` and derive the test fixture from `TerminalEmulator`.
3. **Getter Methods**: Add public getter methods to access the private members.
4. **Test-Specific Interface**: Use a preprocessor macro or separate test interface to expose members.

The **friend class** approach is the most straightforward for unit testing, as it grants the test fixture full access to private members without modifying the class’s public interface or inheritance hierarchy. This is commonly used with Googletest and maintains encapsulation for non-test code.

### Solution
We’ll modify `terminal_emulator.h` to declare the `TerminalEmulatorTest` fixture as a `friend` class, allowing the tests to access private members like `current_attr`. The tests, CMake script, and other files remain unchanged, as the issue is confined to access control.

### Updated Header File
The updated `terminal_emulator.h` adds a `friend class TerminalEmulatorTest` declaration within the `TerminalEmulator` class. No other changes are needed, as the tests already correctly reference the members.

### Explanation of Changes
- **Friend Declaration**:
  - Added `friend class TerminalEmulatorTest;` inside the `TerminalEmulator` class.
  - This grants the `TerminalEmulatorTest` fixture (defined in `terminal_emulator_test.cpp`) full access to private members like `current_attr`, `text_buffer`, `cursor`, `dirty_lines`, and `texture_cache`.
  - The `friend` declaration is safe for testing, as it only affects the test fixture and doesn’t expose private members to other parts of the program.
- **No Other Changes**:
  - The tests in `terminal_emulator_test.cpp` already correctly access the members (e.g., `emulator->current_attr`, `emulator->text_buffer`).
  - The CMake script, `terminal_emulator.cpp`, `main.cpp`, and `terminal_emulator_test.cpp` remain unchanged, as they are unaffected by the access issue.
  - All functionality (per-character colors, `ESC c`, `ESC [ K`, Shift/Control modifiers, etc.) is preserved.

### How to Build and Run
The build process remains the same as in the previous response, using the provided `CMakeLists.txt`. For completeness, here are the steps:

#### Project Structure
Ensure the files are organized as:
```
project_root/
├── CMakeLists.txt
├── src/
│   ├── terminal_emulator.h      # Updated with friend declaration
│   ├── terminal_emulator.cpp
│   ├── main.cpp
│   ├── terminal_emulator_test.cpp
```

#### Prerequisites
- **CMake**: Version 3.14 or higher.
- **SDL2 and SDL2_ttf**:
  - Linux: `sudo apt-get install libsdl2-dev libsdl2-ttf-dev`
  - macOS: `brew install sdl2 sdl2_ttf`
- **Font**: Ensure `/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf` (Linux) or `/System/Library/Fonts/Menlo.ttc` (macOS) exists, or adjust the path in `terminal_emulator.cpp`.
- **Compiler**: C++17-compliant (e.g., `g++`, `clang++`).
- **Internet**: For `FetchContent` to download Googletest.

#### Build Steps
1. **Create Build Directory**:
   ```bash
   mkdir build
   cd build
   ```

2. **Configure**:
   ```bash
   cmake ..
   ```
   On macOS with Homebrew, use:
   ```bash
   cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew
   ```

3. **Build**:
   ```bash
   cmake --build .
   ```

4. **Run Emulator**:
   ```bash
   ./terminal_emulator
   ```

5. **Run Tests**:
   ```bash
   ctest
   ```
   Or:
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

### Testing
- **Verify Compilation**:
  - Ensure `terminal_emulator_test` compiles without errors about private member access.
  - Confirm all 7 tests pass, covering `ESC c`, `ESC [ K`, colors, cursor movement, Shift/Control modifiers, and text buffer insertion.
- **Emulator Functionality**:
  - Run the emulator and test:
    - `echo -e "\033c"` (reset and clear screen).
    - `echo -e "\033[K"` (clear line).
    - Shift+A (inputs 'A'), Ctrl+A (sends 0x01).
    - Tab, arrow keys, resizing, `ls --color`.
- **Check Logs**:
  - If tests fail, review `terminal_emulator_test` output for clues.
  - Verify no new access-related errors (e.g., `private member`).

### Troubleshooting
- **Access Errors Persist**:
  - Ensure `terminal_emulator.h` includes `friend class TerminalEmulatorTest;`.
  - Verify `TerminalEmulatorTest` is the exact class name in `terminal_emulator_test.cpp`.
  - Check for typos in the test file accessing members.
- **SDL2/TTF Issues**:
  - Tests don’t use SDL, but the emulator does. Ensure `SDL2::SDL2` and `SDL2_ttf::SDL2_ttf` are found by CMake.
  - On macOS, use `-DCMAKE_PREFIX_PATH=/opt/homebrew`.
- **Font Path**:
  - If the emulator fails to load the font, modify `terminal_emulator.cpp` or add a CMake option:
    ```cmake
    target_compile_definitions(terminal_emulator PRIVATE FONT_PATH="/path/to/font.ttf")
    ```
- **Googletest**:
  - If `FetchContent` fails, ensure internet access or install Googletest locally and use `find_package(GTest REQUIRED)`.

### Notes
- **Friend Approach**: Chosen for simplicity and common use in Googletest. Alternatives (e.g., getters, protected members) can be implemented if preferred, but they require more changes.
- **Encapsulation**: The `friend` declaration is test-specific and doesn’t affect the emulator’s public interface.
- **Test Coverage**: Tests access all necessary members (`current_attr`, etc.) and verify core logic. Additional tests can be added for edge cases.
- **No Other Changes**: The fix is confined to `terminal_emulator.h`, preserving all functionality and build steps.
