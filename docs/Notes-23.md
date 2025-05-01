To enhance the SDL2-based terminal emulator, we’ll add support for dynamically increasing and decreasing the font size using the key combinations **Ctrl+=** (or **⌘=** on macOS) to increase the font size and **Ctrl+-** (or **⌘-** on macOS) to decrease it. This will involve:

1. **Detecting Key Combinations**:
   - On Linux: Handle **Ctrl+=** and **Ctrl+-** in the `handle_key_event` method.
   - On macOS: Handle **⌘=** and **⌘-** (using the Command key, mapped to `KMOD_GUI` in SDL).
   - Use SDL’s key event handling to detect these combinations.
2. **Adjusting Font Size**:
   - Modify the font size by reloading the font with a new point size using `TTF_OpenFont`.
   - Update `char_width` and `char_height` based on the new font metrics.
   - Adjust the terminal dimensions (`term_cols`, `term_rows`) and window size to reflect the new font size.
3. **Preserving Functionality**:
   - Ensure all existing features (per-character colors, `ESC c`, `ESC [ K`, Shift/Control modifiers, blinking cursor, Tab, special keys, resizing, job control) remain intact.
   - Maintain compatibility with Linux and macOS.
   - Ensure unit tests (using `FRIEND_TEST`) are unaffected, as they don’t test rendering or input handling.

We’ll update the `terminal_emulator.h` and `terminal_emulator.cpp` files to add font size adjustment logic. The unit tests (`terminal_emulator_test.cpp`), `main.cpp`, and `CMakeLists.txt` remain unchanged, as they are not affected by this feature. The `FRIEND_TEST` declarations in `terminal_emulator.h` will be preserved to ensure tests continue to access private members.

### Implementation Approach
1. **Add Font Size Management**:
   - Introduce a `font_size` member to track the current font point size.
   - Add a `change_font_size` method to adjust the font size, reload the font, and update terminal metrics.
2. **Handle Key Events**:
   - In `handle_key_event`, detect **Ctrl+=**/**⌘=** and **Ctrl+-**/**⌘-** using `key.keysym.sym` and `key.keysym.mod`.
   - Call `change_font_size` with a positive or negative increment (e.g., +2 or -2 points).
3. **Update Terminal Metrics**:
   - After changing the font size, recalculate `char_width`, `char_height`, `term_cols`, and `term_rows`.
   - Resize the SDL window and pseudo-terminal (via `ioctl`) to match the new dimensions.
   - Update `text_buffer`, `texture_cache`, and `dirty_lines` to reflect the new terminal size.
4. **Platform-Specific Modifiers**:
   - Use `#ifdef __APPLE__` to detect macOS and check `KMOD_GUI` for Command key presses.
   - Use `KMOD_CTRL` for Linux.
5. **Constraints**:
   - Enforce minimum and maximum font sizes (e.g., 8 and 72 points) to prevent unusable sizes.
   - Ensure the font path remains valid when reloading the font.

### Updated Files
We’ll update `terminal_emulator.h` to add the `font_size` member and `change_font_size` method, and `terminal_emulator.cpp` to implement the font size adjustment and key handling. The other files (`main.cpp`, `terminal_emulator_test.cpp`, `CMakeLists.txt`) are unchanged.

### Key Changes
1. **Header File (`terminal_emulator.h`)**:
   - Added `font_size` (initialized to 16) and `font_path` members to track the current font size and path.
   - Added `change_font_size` method to adjust the font size dynamically.
   - Preserved `FRIEND_TEST` declarations to ensure tests can access private members.

2. **Source File (`terminal_emulator.cpp`)**:
   - **Constructor**: Initializes `font_size` to 16 and sets `font_path` based on platform (`Menlo.ttc` on macOS, `DejaVuSansMono.ttf` on Linux).
   - **handle_key_event**:
     - Added detection for font size key combinations:
       - macOS: **⌘=** (`KMOD_GUI` + `SDLK_EQUALS`) and **⌘-** (`KMOD_GUI` + `SDLK_MINUS`).
       - Linux: **Ctrl+=** (`KMOD_CTRL` + `SDLK_EQUALS`) and **Ctrl+-** (`KMOD_CTRL` + `SDLK_MINUS`).
     - Calls `change_font_size` with `+2` or `-2` to increase or decrease the font size.
   - **change_font_size**:
     - Adjusts `font_size` within bounds (8 to 72 points).
     - Reloads the font with `TTF_OpenFont` using the new size.
     - Updates `char_width` and `char_height` with new font metrics.
     - Recalculates `term_cols` and `term_rows` based on the current window size.
     - Resizes the SDL window with `SDL_SetWindowSize`.
     - Updates the pseudo-terminal size with `ioctl(TIOCSWINSZ)`.
     - Resizes `text_buffer`, `texture_cache`, and `dirty_lines`, and clamps the cursor.
     - Logs the new font size and terminal dimensions.
   - **Fallback Handling**:
     - If font loading fails, reverts to the default size (16).
     - Ensures minimum terminal dimensions (1x1) to prevent crashes.

3. **Preserved Functionality**:
   - All existing features (per-character colors, `ESC c`, `ESC [ K`, Shift/Control modifiers, blinking cursor, Tab, special keys, resizing, job control) are unaffected.
   - Unit tests remain valid, as they don’t test font size or input handling.

### How to Build and Run
The build process is unchanged, using the existing `CMakeLists.txt`. For completeness:

#### Project Structure
Ensure the files are organized as:
```
project_root/
├── CMakeLists.txt
├── src/
│   ├── terminal_emulator.h      # Updated with font_size and change_font_size
│   ├── terminal_emulator.cpp    # Updated with font size handling
│   ├── main.cpp
│   ├── terminal_emulator_test.cpp
```

#### Prerequisites
- **CMake**: Version 3.14 or higher.
- **SDL2 and SDL2_ttf**:
  - Linux: `sudo apt-get install libsdl2-dev libsdl2-ttf-dev`
  - macOS: `brew install sdl2 sdl2_ttf`
- **Font**: Ensure `/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf` (Linux) or `/System/Library/Fonts/Menlo.ttc` (macOS) exists, or adjust `font_path` in `terminal_emulator.cpp`.
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

### Testing
1. **Font Size Adjustment**:
   - **Linux**:
     - Press **Ctrl+=** to increase font size (e.g., from 16 to 18, 20, etc.).
     - Press **Ctrl+-** to decrease font size (e.g., from 16 to 14, 12, etc.).
   - **macOS**:
     - Press **⌘=** to increase font size.
     - Press **⌘-** to decrease font size.
   - Verify:
     - Text size changes visibly.
     - Window size adjusts to maintain the same number of characters (approximately).
     - Terminal dimensions update (check with `stty size`).
     - Cursor remains visible and correctly positioned.
     - No crashes or rendering artifacts occur.
   - Test limits:
     - Increase font size to 72 (maximum); further presses should be ignored.
     - Decrease font size to 8 (minimum); further presses should be ignored.
   - Check logs for messages like:
     ```
     Changed font size to 18, terminal size to 80x24
     ```

2. **Existing Functionality**:
   - Verify all prior features work:
        - `echo -e "\033c"` (reset and clear screen).
     - `echo -e "\033[K"` (clear line).
     - Shift+A (inputs 'A'), Ctrl+A (sends 0x01).
     - Tab, arrow keys, `ls --color`, job control (`sleep 100 &`).
   - Resize the window manually and confirm `stty size` updates.

3. **Unit Tests**:
   - Ensure all 7 tests pass, as they are unaffected by font size changes.
   - Output should show:
     ```
     [==========] Running 7 tests from 1 test suite.
     [----------] 7 tests from TerminalEmulatorTest
     [ RUN      ] TerminalEmulatorTest.EscCResetsStateAndClearsScreen
     [       OK ] TerminalEmulatorTest.EscCResetsStateAndClearsScreen
     ...
     [  PASSED  ] 7 tests.
     ```

### Troubleshooting
- **Font Loading Fails**:
  - If the font fails to load at a new size, check the `font_path` in `terminal_emulator.cpp`.
  - Update to a valid font file (e.g., `LiberationMono.ttf` on Linux).
  - Add a CMake option for custom fonts:
    ```cmake
    target_compile_definitions(terminal_emulator PRIVATE FONT_PATH="/path/to/font.ttf")
    ```
    Modify `TerminalEmulator` constructor to use `FONT_PATH`.
- **Key Combinations Not Detected**:
  - Verify SDL keycodes: `SDLK_EQUALS` (61) and `SDLK_MINUS` (45).
  - Check logs for key events:
    ```
    Key pressed: SDLKey=61, Modifiers=4096  // Cmd+= on macOS
    ```
  - Ensure no conflicting key bindings in your environment.
- **Window/Rendering Issues**:
  - If the window doesn’t resize correctly, verify `SDL_SetWindowSize` and `ioctl(TIOCSWINSZ)` calls.
  - Check for SDL errors in the console.
- **Tests Fail**:
  - Confirm `FRIEND_TEST` declarations match test case names.
  - Tests should be unaffected, as they don’t involve font size or input.

### Notes
- **Font Size Limits**: Set to 8 (minimum) and 72 (maximum) to ensure usability. Adjust in `change_font_size` if needed.
- **Font Path**: Hardcoded for simplicity. Can be made configurable via CMake or environment variables.
- **Platform Detection**: Uses `__APPLE__` for macOS-specific key bindings. Reliable for SDL applications.
- **Performance**: Font reloading is lightweight, and texture cache updates ensure smooth rendering.
- **Test Coverage**: Existing tests cover ANSI parsing and key modifiers but not font size changes (as they involve SDL). Manual testing or SDL mocks can be added if needed.
