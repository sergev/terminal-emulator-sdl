The error indicates that individual test cases in `terminal_emulator_test.cpp` are still unable to access private members like `current_attr` of the `TerminalEmulator` class, despite adding `friend class TerminalEmulatorTest` to `terminal_emulator.h`. This suggests a potential issue with how the `friend` declaration is being interpreted or applied, possibly due to:

1. **Namespace or Scope Mismatch**: The `TerminalEmulatorTest` class in the test file might be in a different namespace or scope, causing the `friend` declaration to reference a different class.
2. **Header Inclusion**: The updated `terminal_emulator.h` might not be correctly included in the test file, or an outdated version is being used.
3. **Compiler Issue**: Some compilers may require explicit forward declaration or namespace specification for friend classes.
4. **Test Fixture Scope**: The test fixture `TerminalEmulatorTest` is defined in the global namespace in `terminal_emulator_test.cpp`, but the `friend` declaration might not be resolving correctly.

Since the `friend class TerminalEmulatorTest` declaration should grant access to private members, the issue likely stems from a subtle mismatch or configuration error. To resolve this, we’ll:

1. **Explicitly Forward-Declare the Test Fixture**: Add a forward declaration of `TerminalEmulatorTest` in `terminal_emulator.h` to ensure the `friend` declaration resolves correctly.
2. **Verify Header Inclusion**: Ensure the test file includes the updated `terminal_emulator.h`.
3. **Simplify Test Access**: If needed, add getter methods as a fallback to access private members, though the `friend` approach should work.
4. **Check Test File**: Verify that the test file is using the correct class name and accessing members properly.

### Updated Header File
We’ll update `terminal_emulator.h` to include a forward declaration of `TerminalEmulatorTest` before the `TerminalEmulator` class, ensuring the `friend` declaration is unambiguous. This is a common fix for friend class issues in C++ when the test fixture is defined in a separate file.

### Verification of Other Files
To ensure the issue is fully resolved, let’s confirm that the test file and other components are correctly set up:

- **Test File (`terminal_emulator_test.cpp`)**:
  - The test fixture is named `TerminalEmulatorTest`, matching the `friend` declaration.
  - Tests access private members like `emulator->current_attr`, `emulator->text_buffer`, etc., which should now be allowed.
  - No changes are needed, as the test file is correct (based on the previous version).
- **CMakeLists.txt**:
  - The build script correctly includes `terminal_emulator.h` and links against Googletest, SDL2, and SDL2_ttf.
  - No changes are needed.
- **terminal_emulator.cpp** and `main.cpp`:
  - These files are unaffected by the access issue, as they don’t involve test-specific access to private members.
  - No changes are needed.

### Alternative Approach: Getter Methods
If the `friend` declaration still fails (e.g., due to compiler quirks or project constraints), we can add getter methods to `TerminalEmulator` as a fallback. For example:

```cpp
public:
    const CharAttr& get_current_attr() const { return current_attr; }
    const std::vector<std::vector<Char>>& get_text_buffer() const { return text_buffer; }
    const Cursor& get_cursor() const { return cursor; }
    const std::vector<bool>& get_dirty_lines() const { return dirty_lines; }
```

Then update the tests to use `emulator->get_current_attr()` instead of `emulator->current_attr`. However, the `friend` approach is cleaner for testing and should work with the forward declaration, so we’ll prioritize it.

### How to Build and Run
The build process remains identical to the previous response, but we’ll reiterate for clarity:

#### Project Structure
Ensure the files are organized as:
```
project_root/
├── CMakeLists.txt
├── src/
│   ├── terminal_emulator.h      # Updated with forward declaration
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
  - Ensure `terminal_emulator_test` compiles without errors like `'current_attr' is a private member`.
  - Confirm all 7 tests pass, covering `ESC c`, `ESC [ K`, colors, cursor movement, Shift/Control modifiers, and text buffer insertion.
- **Emulator Functionality**:
  - Run the emulator and test:
    - `echo -e "\033c"` (reset and clear screen).
    - `echo -e "\033[K"` (clear line).
    - Shift+A (inputs 'A'), Ctrl+A (sends 0x01).
    - Tab, arrow keys, resizing, `ls --color`.
- **Check Logs**:
  - If tests fail, review `terminal_emulator_test` output.
  - Verify no access-related errors remain.

### Troubleshooting
- **Access Errors Persist**:
  - **Verify Header**: Ensure `terminal_emulator.h` includes the forward declaration (`class TerminalEmulatorTest;`) and `friend class TerminalEmulatorTest;`.
  - **Check Test File**: Confirm `terminal_emulator_test.cpp` defines `class TerminalEmulatorTest` in the global namespace, as shown in the previous version.
  - **Rebuild**: Run `cmake ..` and `cmake --build .` to ensure the updated header is used.
  - **Compiler**: Test with a different compiler (e.g., `g++` vs. `clang++`) to rule out compiler-specific issues.
  - **Fallback to Getters**: If the issue persists, I can provide a version with getter methods. Add to `TerminalEmulator`:
    ```cpp
    public:
        const CharAttr& get_current_attr() const { return current_attr; }
        const std::vector<std::vector<Char>>& get_text_buffer() const { return text_buffer; }
        const Cursor& get_cursor() const { return cursor; }
        const std::vector<bool>& get_dirty_lines() const { return dirty_lines; }
    ```
    Update tests, e.g., `emulator->current_attr` to `emulator->get_current_attr()`.
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
- **Friend Declaration**: The forward declaration ensures the `friend` statement resolves correctly, a common fix for cross-file friend classes.
- **Minimal Changes**: Only `terminal_emulator.h` is updated, preserving all functionality and build steps.
- **Test Coverage**: Tests now access private members (`current_attr`, etc.) and verify core logic. Additional tests can be added if needed.
- **Encapsulation**: The `friend` approach is test-specific and maintains encapsulation for non-test code.
