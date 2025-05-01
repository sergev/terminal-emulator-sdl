The persistent issue with tests unable to access private members like `current_attr` in `TerminalEmulator` suggests that the `friend class TerminalEmulatorTest` declaration, even with a forward declaration, is not resolving correctly. This could be due to compiler-specific behavior, namespace issues, or limitations in how the `friend` declaration is processed across translation units. Since the `FRIEND_TEST` macro from Googletest is specifically designed to address such issues by declaring individual test cases as friends of the class, we’ll use it to grant access to private members for each test case in `terminal_emulator_test.cpp`.

The `FRIEND_TEST` macro, defined in `<gtest/gtest_prod.h>`, allows specific test cases (e.g., `TEST_F(TerminalEmulatorTest, EscCResetsStateAndClearsScreen)`) to be declared as friends, enabling them to access private members like `current_attr`, `text_buffer`, `cursor`, and `dirty_lines`. This approach is more granular than `friend class` and avoids issues with class-level friend declarations.

### Solution
We’ll update `terminal_emulator.h` to:
1. Include `<gtest/gtest_prod.h>` for the `FRIEND_TEST` macro.
2. Replace the `friend class TerminalEmulatorTest` with `FRIEND_TEST` declarations for each test case in `terminal_emulator_test.cpp`.
3. Remove the forward declaration of `TerminalEmulatorTest`, as it’s no longer needed.

The test file (`terminal_emulator_test.cpp`) and other files (`terminal_emulator.cpp`, `main.cpp`, `CMakeLists.txt`) remain unchanged, as they are correct and unaffected by the access issue. The `FRIEND_TEST` macro ensures that each test case can access private members without altering the test logic.

### Updated Header File
The updated `terminal_emulator.h` uses `FRIEND_TEST` to declare each test case as a friend of `TerminalEmulator`. The test cases are identified from `terminal_emulator_test.cpp`: `EscCResetsStateAndClearsScreen`, `EscKClearsLine`, `EscMSetsColors`, `CursorMovement`, `ShiftModifier`, `ControlModifier`, and `TextBufferInsertion`.

### Explanation of Changes
- **Added `<gtest/gtest_prod.h>`**:
  - Included at the top of `terminal_emulator.h` to provide the `FRIEND_TEST` macro.
  - This header is part of Googletest and safe to include, as it’s designed for test-specific friend declarations.
- **Replaced Friend Class with FRIEND_TEST**:
  - Removed `friend class TerminalEmulatorTest` and the forward declaration `class TerminalEmulatorTest`.
  - Added `FRIEND_TEST(TerminalEmulatorTest, <TestName>)` for each test case:
    - `EscCResetsStateAndClearsScreen`
    - `EscKClearsLine`
    - `EscMSetsColors`
    - `CursorMovement`
    - `ShiftModifier`
    - `ControlModifier`
    - `TextBufferInsertion`
  - Each `FRIEND_TEST` macro expands to a `friend` declaration for the specific test case, granting access to private members like `current_attr`, `text_buffer`, `cursor`, and `dirty_lines`.
- **No Changes to Tests**:
  - The test file (`terminal_emulator_test.cpp`) already accesses members correctly (e.g., `emulator->current_attr`).
  - The `FRIEND_TEST` declarations ensure these accesses are now legal.
- **No Changes to Other Files**:
  - `terminal_emulator.cpp`, `main.cpp`, and `CMakeLists.txt` are unaffected, as the issue is confined to access control in the header.
  - All functionality (per-character colors, `ESC c`, `ESC [ K`, Shift/Control modifiers, etc.) remains intact.

### Verification
The tests in `terminal_emulator_test.cpp` should now compile without errors like `'current_attr' is a private member`. The `FRIEND_TEST` macro explicitly allows each test case to access private members, resolving the issue reported at line 25 of `terminal_emulator_test.cpp` (likely in the `EscCResetsStateAndClearsScreen` test, which accesses `current_attr`).

### How to Build and Run
The build process is unchanged from the previous response, as the CMake script and other files are unaffected. For completeness:

#### Project Structure
Ensure the files are organized as:
```
project_root/
├── CMakeLists.txt
├── src/
│   ├── terminal_emulator.h      # Updated with FRIEND_TEST
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
   On macOS with Homebrew:
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
  - Ensure `terminal_emulator_test` compiles without errors like `'current_attr' is a private member` at line 25 or elsewhere.
  - Confirm all 7 tests pass, covering `ESC c`, `ESC [ K`, colors, cursor movement, Shift/Control modifiers, and text buffer insertion.
- **Emulator Functionality**:
  - Run the emulator and test:
    - `echo -e "\033c"` (reset and clear screen).
    - `echo -e "\033[K"` (clear line).
    - Shift+A (inputs 'A'), Ctrl+A (sends 0x01).
    - Tab, arrow keys, resizing, `ls --color`.
- **Check Logs**:
  - If tests fail, review `terminal_emulator_test` output for errors unrelated to access (e.g., logic errors).
  - Verify no private member access errors remain.

### Troubleshooting
- **Access Errors Persist**:
  - **Verify FRIEND_TEST**: Ensure `terminal_emulator.h` includes `FRIEND_TEST` for all test cases listed above, matching the exact names in `terminal_emulator_test.cpp`.
  - **Check Inclusion**: Confirm `<gtest/gtest_prod.h>` is included and `terminal_emulator.h` is updated in the build.
  - **Rebuild**: Run `cmake ..` and `cmake --build .` to ensure the updated header is used.
  - **Test Names**: Verify that the test case names in `FRIEND_TEST` exactly match those in `terminal_emulator_test.cpp` (e.g., `EscCResetsStateAndClearsScreen`).
  - **Compiler**: Test with `g++` or `clang++` to rule out compiler-specific issues.
  - **Fallback to Getters**: If `FRIEND_TEST` fails, we can add getter methods as a last resort:
    ```cpp
    public:
        const CharAttr& get_current_attr() const { return current_attr; }
        const std::vector<std::vector<Char>>& get_text_buffer() const { return text_buffer; }
        const Cursor& get_cursor() const { return cursor; }
        const std::vector<bool>& get_dirty_lines() const { return dirty_lines; }
    ```
    Update tests to use `emulator->get_current_attr()` instead of `emulator->current_attr`.
- **SDL2/TTF Issues**:
  - Ensure libraries are installed and found by CMake.
  - On macOS, use `-DCMAKE_PREFIX_PATH=/opt/homebrew`.
- **Font Path**:
  - If the emulator fails to load the font, modify `terminal_emulator.cpp` or add a CMake option:
    ```cmake
    target_compile_definitions(terminal_emulator PRIVATE FONT_PATH="/path/to/font.ttf")
    ```
- **Googletest**:
  - Ensure `FetchContent` downloads Googletest (internet required).
  - Alternatively, install Googletest locally and use `find_package(GTest REQUIRED)`.

### Notes
- **FRIEND_TEST Macro**: Specifically designed for Googletest to grant individual test cases access to private members, avoiding issues with class-level friend declarations.
- **Granular Access**: Each test case is explicitly declared as a friend, ensuring only the necessary tests access private members.
- **Minimal Changes**: Only `terminal_emulator.h` is updated, preserving all functionality and build steps.
- **Test Coverage**: Tests access private members (`current_attr`, etc.) and verify core logic. Additional tests can be added if needed.
- **Encapsulation**: `FRIEND_TEST` is test-specific and maintains encapsulation for non-test code.
