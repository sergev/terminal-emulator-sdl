To improve the modularity of the terminal emulator, we’ll split the `TerminalEmulator` class into two distinct classes with clear responsibilities:

1. **SystemInterface** (First Class):
   - Handles all platform-specific and low-level operations:
     - SDL2 interface (window creation, rendering, event handling).
     - Pseudo-terminal (PTY) management.
     - Fork/exec for the child process (shell).
     - Unix signal handling (e.g., SIGWINCH, SIGINT).
   - Manages font size changes (Ctrl+=/⌘= and Ctrl+-/⌘-).
   - Interacts with the second class to process input and update the display.

2. **TerminalLogic** (Second Class):
   - Handles all terminal logic independent of SDL2 and Unix:
     - Processing of ANSI escape sequences (e.g., `ESC c`, `ESC [ K`, colors, cursor movement).
     - Key input processing (e.g., Shift, Control modifiers, printable characters).
     - Maintains the text buffer, cursor, and character attributes.
   - Knows nothing about SDL2, PTY, or Unix signals, ensuring portability and testability.

### Design Approach
- **SystemInterface**:
  - Owns SDL resources (`window`, `renderer`, `font`), PTY (`master_fd`, `child_pid`), and signal handlers.
  - Manages the rendering pipeline (texture cache, spans, cursor).
  - Reads input from the PTY and forwards it to `TerminalLogic` for processing.
  - Sends key inputs to `TerminalLogic` for interpretation and forwards the resulting strings to the PTY.
  - Calls `TerminalLogic` methods to update the text buffer and cursor based on ANSI sequences.
- **TerminalLogic**:
  - Manages the text buffer (`text_buffer`), cursor (`cursor`), character attributes (`current_attr`), and ANSI state (`state`, `ansi_seq`).
  - Processes raw input bytes (from PTY) to handle ANSI sequences and update the buffer.
  - Processes key events (abstracted as key codes and modifiers) to generate input strings (e.g., `\n`, `\033[A`).
  - Provides methods for `SystemInterface` to query the text buffer, cursor, and dirty lines for rendering.
- **Interface Between Classes**:
  - `SystemInterface` owns a `TerminalLogic` instance and calls its methods to process input and keys.
  - `TerminalLogic` exposes methods like `process_input`, `process_key`, and getters for `text_buffer`, `cursor`, etc.
  - No SDL2 or Unix dependencies in `TerminalLogic`, making it easier to test and potentially reusable.
- **Preserve Existing Functionality**:
  - Maintains all features: per-character colors, `ESC c`, `ESC [ K`, Shift/Control modifiers, blinking cursor, Tab, special keys, resizing, job control, font size changes (Ctrl+=/⌘= and Ctrl+-/⌘-).
  - Ensures unit tests (`terminal_emulator_test.cpp`) remain valid, updating them to test `TerminalLogic` instead of `TerminalEmulator`.
- **File Structure**:
  - `system_interface.h` and `system_interface.cpp`: Define and implement `SystemInterface`.
  - `terminal_logic.h` and `terminal_logic.cpp`: Define and implement `TerminalLogic`.
  - `main.cpp`: Creates `SystemInterface` and runs it.
  - `terminal_logic_test.cpp`: Updated unit tests for `TerminalLogic`, using `FRIEND_TEST`.
  - `CMakeLists.txt`: Updated to include new source files.

### Updated Files
We’ll create new header and source files for the two classes, update the main program, revise the unit tests to target `TerminalLogic`, and modify the CMake script to include the new files. The `FRIEND_TEST` approach will be used in `terminal_logic.h` to allow tests to access private members.

### Project Structure
Ensure the files are organized as:
```
project_root/
├── CMakeLists.txt
├── src/
│   ├── system_interface.h
│   ├── system_interface.cpp
│   ├── terminal_logic.h
│   ├── terminal_logic.cpp
│   ├── main.cpp
│   ├── terminal_logic_test.cpp
```

### Key Changes
1. **Class Separation**:
   - `SystemInterface`:
     - Handles SDL2 (window, renderer, font), PTY (master_fd, child_pid), and signals.
     - Manages font size changes (Ctrl+=/⌘= and Ctrl+-/⌘-).
     - Renders text using `terminal_logic`’s text buffer and cursor.
   - `TerminalLogic`:
     - Handles ANSI escape sequences (`ESC c`, `ESC [ K`, colors, cursor movement).
     - Processes key inputs (Shift, Control, special keys) without SDL dependencies.
     - Uses `uint8_t` for color components instead of `SDL_Color` to avoid SDL dependency.
     - Returns dirty rows from `process_input` for `SystemInterface` to update `dirty_lines`.

2. **TerminalLogic**:
   - Constructor initializes `text_buffer`, `cursor`, and `current_attr`.
   - `resize` updates `term_cols`, `term_rows`, and `text_buffer`.
   - `process_input` processes PTY input, returns dirty row indices.
   - `process_key` maps keycodes and modifiers to input strings, using raw SDL keycodes (e.g., 13 for SDLK_RETURN) and modifier flags (0x1000 for KMOD_CTRL).
   - Getters for `text_buffer` and `cursor` allow `SystemInterface` to render.

3. **SystemInterface**:
   - Owns a `TerminalLogic` instance, initialized with the same `cols` and `rows`.
   - Forwards PTY input to `terminal_logic.process_input` and updates `dirty_lines`.
   - Converts SDL key events to `terminal_logic.process_key` calls.
   - Handles Ctrl+C/D signals directly (as they affect the child process).
   - Updates `texture_cache` and `dirty_lines` based on `terminal_logic`’s state.

4. **Tests**:
   - Renamed to `TerminalLogicTest` and updated to test `TerminalLogic` instead of `TerminalEmulator`.
   - Adjusted member access (e.g., `fg_r` instead of `fg.r`) due to `CharAttr` changes.
   - Tests remain functionally identical, covering `ESC c`, `ESC [ K`, colors, cursor, modifiers, and buffer insertion.
   - `FRIEND_TEST` declarations moved to `terminal_logic.h`.

5. **CMake**:
   - Updated to include `system_interface.cpp` and `terminal_logic.cpp` for the emulator.
   - Tests now use `terminal_logic.cpp` and `terminal_logic_test.cpp`.

### How to Build and Run
#### Prerequisites
- **CMake**: Version 3.14 or higher.
- **SDL2 and SDL2_ttf**:
  - Linux: `sudo apt-get install libsdl2-dev libsdl2-ttf-dev`
  - macOS: `brew install sdl2 sdl2_ttf`
- **Font**: Ensure `/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf` (Linux) or `/System/Library/Fonts/Menlo.ttc` (macOS) exists, or adjust `font_path` in `system_interface.cpp`.
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
   ./terminal_logic_test
   ```
   Expected output:
   ```
   [==========] Running 7 tests from 1 test suite.
   [----------] 7 tests from TerminalLogicTest
   [ RUN      ] TerminalLogicTest.EscCResetsStateAndClearsScreen
   [       OK ] TerminalLogicTest.EscCResetsStateAndClearsScreen
   ...
   [  PASSED  ] 7 tests.
   ```

### Testing
1. **Font Size Adjustment**:
   - Linux: **Ctrl+=** (increase), **Ctrl+-** (decrease).
   - macOS: **⌘=** (increase), **⌘-** (decrease).
   - Verify text size changes, window resizes, and `stty size` updates.
   - Test limits (8 to 72 points).

2. **Terminal Functionality**:
   - Test:
     - `echo -e "\033c"` (reset and clear).
     - `echo -e "\033[K"` (clear line).
     - Shift+A (‘A’), Ctrl+A (0x01), Tab, arrow keys, `ls --color`.
     - Job control (`sleep 100 &`).
   - Resize window and check `stty size`.

3. **Unit Tests**:
   - Confirm all 7 tests pass, covering `TerminalLogic`’s ANSI parsing, key processing, and buffer management.

### Troubleshooting
- **Font Issues**:
  - Update `font_path` in `system_interface.cpp` or add CMake option:
    ```cmake
    target_compile_definitions(terminal_emulator PRIVATE FONT_PATH="/path/to/font.ttf")
    ```
- **Key Detection**:
  - Check logs for key events (e.g., `Keycode=61, Modifiers=4096` for Cmd+=).
  - Verify SDL keycodes in `terminal_logic.cpp`.
- **Rendering Issues**:
  - Ensure `dirty_lines` aligns with `terminal_logic`’s dirty rows.
  - Check SDL errors in logs.
- **Test Failures**:
  - Verify `FRIEND_TEST` names match test cases.
  - Check `CharAttr` member access (`fg_r` vs. `fg.r`).

### Notes
- **Modularity**: `TerminalLogic` is now independent of SDL2/Unix, improving testability and portability.
- **Tests**: Updated to focus on `TerminalLogic`, preserving coverage.
- **Performance**: Minimal overhead from class separation.
- **Extensibility**: `TerminalLogic` can be reused in non-SDL contexts.

If you encounter issues (e.g., compilation errors, crashes, or feature regressions) or want further enhancements (e.g., more ANSI sequences, additional tests), please share the error output or requirements, and I’ll refine the solution!
