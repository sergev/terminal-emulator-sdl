The issue is that `terminal_logic.h` still contains SDL-related types in the `TextSpan` structure, specifically `SDL_Color` in `CharAttr`, which introduces an SDL dependency and causes GCC compilation errors in `terminal_logic.cpp` (and potentially `system_interface.cpp` when including `terminal_logic.h`). The goal was to make `TerminalLogic` completely independent of SDL2 and Unix, so we need to remove all SDL-related types from `terminal_logic.h` and `terminal_logic.cpp`. Additionally, we’ll ensure `system_interface.cpp` handles all SDL-specific logic correctly to avoid GCC errors.

### Problem Analysis
- **SDL Dependency in `terminal_logic.h`**:
  - The `CharAttr` struct uses `SDL_Color` for `fg` and `bg`, which requires `<SDL2/SDL.h>`.
  - This violates the requirement that `TerminalLogic` knows nothing about SDL2.
  - Including `<SDL2/SDL.h>` in `terminal_logic.h` causes GCC errors when compiling `terminal_logic.cpp` or `terminal_logic_test.cpp` if SDL2 headers are not properly configured or if there’s a mismatch in include paths.
- **Potential GCC Errors**:
  - Errors like `SDL_Color: unknown type name` or `SDL2/SDL.h: No such file or directory` in `terminal_logic.cpp`.
  - Linker errors if SDL2 libraries are not correctly linked for `terminal_logic_test`.
  - Conflicts in `system_interface.cpp` due to redundant SDL includes or type mismatches.
- **Remaining SDL Stuff**:
  - The `TextSpan` struct in `terminal_logic.h` includes a `texture` field typed as `void*`, which is meant for SDL textures but doesn’t directly depend on SDL. However, `CharAttr`’s use of `SDL_Color` is the primary issue.
  - `SystemInterface` correctly handles SDL rendering, but we need to ensure `TerminalLogic`’s data structures are SDL-free.

### Solution
We’ll update `terminal_logic.h` and `terminal_logic.cpp` to:
1. Replace `SDL_Color` in `CharAttr` with `uint8_t` fields (`fg_r`, `fg_g`, `fg_b`, `fg_a`, `bg_r`, `bg_g`, `bg_b`, `bg_a`), as already partially done in the previous version.
2. Ensure `TextSpan` remains SDL-free by keeping `texture` as `void*`.
3. Remove any SDL2 includes (`<SDL2/SDL.h>`, `<SDL2/SDL_ttf.h>`) from `terminal_logic.h` and `terminal_logic.cpp`.
4. Update `system_interface.cpp` to convert `TerminalLogic`’s `CharAttr` to `SDL_Color` for rendering.
5. Verify that `TerminalLogic` has no SDL or Unix dependencies.
6. Adjust the unit tests (`terminal_logic_test.cpp`) to match the updated `CharAttr` structure, if necessary.
7. Ensure the CMake configuration correctly links SDL2 only for `terminal_emulator`, not `terminal_logic_test`.

The previous version of `terminal_logic.h` already uses `uint8_t` for `CharAttr`, but we’ll double-check and remove any residual SDL dependencies. We’ll also inspect `system_interface.cpp` for any SDL-related issues that might cause GCC errors (e.g., missing includes, incorrect type conversions).

### Updated Files
We’ll update `terminal_logic.h`, `terminal_logic.cpp`, `system_interface.cpp`, and `terminal_logic_test.cpp` to ensure `TerminalLogic` is SDL-free and GCC errors are resolved. The `main.cpp` and `CMakeLists.txt` files remain correct but will be included for completeness.

#### Updated Header File: `terminal_logic.h`
This version ensures no SDL dependencies by using `uint8_t` for colors and removing SDL includes. The `TextSpan` struct retains `void* texture` for `SystemInterface` to use as an SDL texture.

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
1. **terminal_logic.h and terminal_logic.cpp**:
   - Removed all SDL includes (`<SDL2/SDL.h>`, `<SDL2/SDL_ttf.h>`).
   - `CharAttr` uses `uint8_t` for color components, ensuring no SDL dependency.
   - `TextSpan` keeps `void* texture`, which is SDL-free and managed by `SystemInterface`.
   - No Unix-specific includes (e.g., `<pty.h>`, `<util.h>`), as `TerminalLogic` doesn’t handle PTY or signals.

2. **system_interface.cpp**:
   - Converts `CharAttr` to `SDL_Color` in `update_texture_cache` and `render_spans`:
     ```cpp
     SDL_Color fg = {current_span_attr.fg_r, current_span_attr.fg_g, current_span_attr.fg_b, current_span_attr.fg_a};
     ```
   - Explicitly casts `void* texture` to `SDL_Texture*` for SDL operations:
     ```cpp
     SDL_DestroyTexture(static_cast<SDL_Texture*>(span.texture));
     ```
   - Includes all necessary SDL headers (`<SDL2/SDL.h>`, `<SDL2/SDL_ttf.h>`) to avoid GCC errors.
   - Handles all SDL and Unix logic (window, renderer, PTY, signals).

3. **terminal_logic_test.cpp**:
   - Uses `fg_r`, `fg_g`, etc., consistent with `CharAttr`.
   - No SDL dependencies, ensuring tests compile without SDL2 libraries.

4. **CMakeLists.txt**:
   - Ensures `terminal_emulator` links against SDL2 and SDL2_ttf.
   - `terminal_logic_test` only links against Googletest, avoiding SDL2 dependencies.

### How to Build and Run
#### Prerequisites
- **CMake**: Version 3.14 or higher.
- **SDL2 and SDL2_ttf**:
  - Linux: `sudo apt-get install libsdl2-dev libsdl2-ttf-dev`
  - macOS: `brew install sdl2 sdl2_ttf`
- **Font**: Ensure `/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf` (Linux) or `/System/Library/Fonts/Menlo.ttc` (macOS) exists, or adjust `font_path` in `system_interface.cpp`.
- **Compiler**: GCC with C++17 support (e.g., `g++`).
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
   - Verify text size changes, window resizes, `stty size` updates.
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
- **GCC Errors**:
  - If errors like `SDL_Color: unknown type name` persist, ensure `terminal_logic.h` and `terminal_logic.cpp` have no SDL includes.
  - Verify SDL2 headers are found:
    ```bash
    pkg-config --cflags --libs sdl2 SDL2_ttf
    ```
    Update `CMAKE_PREFIX_PATH` if needed.
  - Share the exact GCC error output for precise diagnosis.
- **Font Issues**:
  - Update `font_path` or add CMake option:
    ```cmake
    target_compile_definitions(terminal_emulator PRIVATE FONT_PATH="/path/to/font.ttf")
    ```
- **Key Detection**:
  - Check logs for key events (e.g., `Keycode=61, Modifiers=4096` for Cmd+=).
- **Rendering Issues**:
  - Verify `CharAttr` to `SDL_Color` conversion in `update_texture_cache`.

### Notes
- **SDL-Free TerminalLogic**: Now completely independent of SDL2/Unix, improving portability and testability.
- **GCC Compatibility**: Explicit includes and type casts in `system_interface.cpp` resolve compilation issues.
- **Preserved Functionality**: All features (font size, ANSI sequences, key modifiers, tests) remain intact.
- **Extensibility**: `TerminalLogic` can be reused in non-SDL contexts.
