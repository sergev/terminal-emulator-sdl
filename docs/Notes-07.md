# Version 7

To implement dynamic resizing in the SDL2-based terminal emulator, we need to allow the slave pseudo-terminal's size (rows and columns) to adjust dynamically based on the SDL2 window's size, which is measured in pixels and converted to character units using the font's metrics. This involves:
1. Updating the slave terminal size (`TIOCSWINSZ`) whenever the SDL2 window is resized, calculating the new rows and columns from the window's pixel dimensions and the font's character width/height.
2. Modifying the `SIGWINCH` handler to use the current window size instead of fixed `max_cols` and `max_rows`.
3. Adjusting the text buffer to accommodate the new terminal dimensions, ensuring proper rendering within the updated bounds.
4. Handling SDL2 window resize events (`SDL_WINDOWEVENT_RESIZED`) to trigger the size update and notify the shell.
5. Maintaining all existing functionality (key handling, slave output processing, job control, Linux/macOS compatibility).

### Approach
- **Window Size Calculation**: On window resize, get the SDL2 window's pixel dimensions (`SDL_GetWindowSize`) and compute the number of columns and rows by dividing by the font's `char_width` and `char_height`.
- **Slave Terminal Update**: Use `ioctl(master_fd, TIOCSWINSZ)` to set the slave terminal's size and send `SIGWINCH` to the child process to notify the shell.
- **Text Buffer**: Dynamically adjust the text buffer's line lengths and total lines to match the current terminal dimensions, trimming or padding as needed.
- **Rendering**: Update the rendering loop to use the current rows and columns for text display.
- **Event Handling**: Process `SDL_WINDOWEVENT_RESIZED` to trigger the size update immediately.

### Updated Code
The following C++ code extends the previous SDL2 terminal emulator to support dynamic resizing while preserving all prior functionality. It remains compatible with Linux and macOS.

### Key Changes
1. **Dynamic Terminal Size**:
   - Replaced fixed `max_cols` and `max_rows` with dynamic `term_cols` and `term_rows`, initialized to 80x24 but updated on resize.
   - Added `update_terminal_size` function to:
     - Get the SDL window's pixel size (`SDL_GetWindowSize`).
     - Calculate `term_cols = win_width / char_width` and `term_rows = win_height / char_height`.
     - Set the slave terminal size using `ioctl(master_fd, TIOCSWINSZ)`.
     - Send `SIGWINCH` to the child process.
     - Adjust the text buffer to fit the new `term_cols` and `term_rows`.

2. **SIGWINCH Handler**:
   - Updated `handle_sigwinch` to call `update_terminal_size`, which uses the current SDL window size instead of fixed dimensions.
   - Triggered by both `SDL_WINDOWEVENT_RESIZED` and external `SIGWINCH` signals (e.g., if the window manager sends them).

3. **Text Buffer Management**:
   - Modified the text buffer to limit each line to `term_cols` characters and the total lines to `term_rows`.
   - On resize, trimmed oversized lines and removed excess lines to fit the new dimensions.

4. **Rendering**:
   - Updated `render_text` to use `term_rows` for the rendering loop, ensuring only the current terminal height is displayed.
   - Kept the same rendering logic (white text on black background).

5. **Event Handling**:
   - Ensured `SDL_WINDOWEVENT_RESIZED` triggers `update_terminal_size` immediately, updating the slave terminal and text buffer.

### How to Compile and Run
#### Prerequisites
- SDL2 and SDL_ttf installed:
  - **Linux**: `sudo apt-get install libsdl2-dev libsdl2-ttf-dev`
  - **macOS**: `brew install sdl2 sdl2_ttf`
- Monospaced font available:
  - Linux: `/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf` (or install `fonts-dejavu`).
  - macOS: `/System/Library/Fonts/Menlo.ttc` (built-in).
  - Adjust the font path in the code if needed.

#### On macOS
1. Save the code as `terminal_emulator_sdl.cpp`.
2. Compile with `clang++`, linking SDL2 and SDL_ttf:
   ```bash
   clang++ -o terminal_emulator_sdl terminal_emulator_sdl.cpp -I/usr/local/include -L/usr/local/lib -lSDL2 -lSDL2_ttf
   ```
   (Adjust paths if Homebrew installed SDL2 elsewhere, e.g., `/opt/homebrew`.)
3. Run the program:
   ```bash
   ./terminal_emulator_sdl
   ```

#### On Linux
1. Save the code as `terminal_emulator_sdl.cpp`.
2. Compile with `g++`, linking SDL2 and SDL_ttf:
   ```bash
   g++ -o terminal_emulator_sdl terminal_emulator_sdl.cpp -lSDL2 -lSDL2_ttf
   ```
3. Run the program:
   ```bash
   ./terminal_emulator_sdl
   ```

### Expected Behavior
- **SDL Window**:
  - Opens a window (initially 80x24 characters) with a `/bin/sh` prompt.
  - Resizing the window dynamically adjusts the slave terminal size (rows and columns).
- **Dynamic Resizing**:
  - Resizing the SDL window updates `term_cols` and `term_rows` based on pixel dimensions divided by `char_width` and `char_height`.
  - The slave terminal size is updated via `ioctl(TIOCSWINSZ)`, and the shell is notified with `SIGWINCH`.
  - Commands like `stty size` reflect the new dimensions (e.g., `30 100` after resizing).
- **Text Buffer**:
  - Lines are trimmed to `term_cols`, and the buffer is limited to `term_rows` lines.
  - Output adjusts to the new size without distortion.
- **Key Input**:
  - Typing, Enter, Backspace, Ctrl+C, and Ctrl+D work as before.
- **Slave Output**:
  - Shell output (e.g., `ls`, `echo`) displays correctly, respecting the new terminal size.
- **Job Control**:
  - Job control (e.g., `sleep 100 &`, `fg`, `bg`) and signal handling (Ctrl+C) work as before.
  - No "can't access tty" warnings.

### Testing
1. **Dynamic Resizing**:
   - Resize the SDL window (drag or maximize) and run `stty size` to verify the reported rows and columns match the window size (e.g., `30 100` for a larger window).
   - Open `vim` or `top` and resize the window to confirm they adjust to the new dimensions.
   - Check debug output (e.g., `Updated slave size to 100x30`) to confirm `ioctl` success.
2. **Text Rendering**:
   - Run `ls -l` or `cat` to ensure output wraps and scrolls correctly within the new terminal size.
   - Test with `printf "A\nB\nC\n"` to verify line handling.
3. **Key Input**:
   - Type commands and use Backspace, Enter, Ctrl+C to confirm input works.
   - Press Ctrl+D to exit cleanly.
4. **Job Control**:
   - Run `sleep 100 &` and use `fg` or `bg` to manage jobs.
   - Press Ctrl+Z to suspend a command and resume it.

### Notes
- **Font Path**: If the font fails to load, update the `TTF_OpenFont` path to a valid monospaced font (e.g., `LiberationMono.ttf` on Linux or another `.ttc` on macOS).
- **Limitations**:
  - Still handles only basic ASCII; ANSI escape sequences (colors, cursor movement) are not supported. I can add this if needed.
  - Special keys (e.g., arrow keys, Tab) are not fully handled; only basic input is supported.
  - Resizing may cause minor visual glitches if the shell outputs rapidly; a more robust renderer could be added.
- **Performance**: The rendering is simple and may flicker on rapid updates. For production, consider texture caching or double-buffering.
- **Debug Output**: Check `std::cerr` messages for resize events or errors (e.g., `ioctl` failures).

