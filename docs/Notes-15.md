# Version 14

To implement the ANSI escape sequence `ESC c` (reset to defaults and clear screen) in the SDL2-based terminal emulator, we need to:
1. **Handle `ESC c`**:
   - Recognize the `ESC c` sequence (`\033c`) in the ANSI parsing logic.
   - Reset all relevant terminal state to defaults, including:
     - Colors: Set `current_attr` to default (white foreground, black background).
     - Cursor position: Move to (0, 0).
   - Clear the entire screen by resetting the text buffer to spaces with default attributes.
2. **Preserve Functionality**:
   - Maintain per-character foreground and background color support, Tab key handling, blinking cursor, special key support, dynamic resizing, antialiased text, double-buffered rendering, and job control.
   - Ensure compatibility with Linux and macOS.
3. **Integration**:
   - Update the ANSI parsing state machine to detect `ESC c` as a distinct sequence (not a CSI sequence like `ESC [...`).
   - Clear the texture cache and mark all lines as dirty to trigger a full redraw with the cleared screen.

### Approach
- **Parsing `ESC c`**:
  - In the `AnsiState::ESCAPE` state, check for `c` after `\033` and process it immediately.
  - Avoid treating it as a CSI sequence (which expects `[` after `\033`).
- **Reset and Clear**:
  - Reset `current_attr` to `{fg: {255, 255, 255, 255}, bg: {0, 0, 0, 255}}`.
  - Set `cursor` to `{row: 0, col: 0}`.
  - Clear `text_buffer` by filling all lines with spaces and default attributes.
  - Clear `texture_cache` and mark all `dirty_lines` as true to force a redraw.
- **Rendering**:
  - Rely on existing rendering logic to display the cleared screen with per-character colors.
  - Ensure the blinking cursor appears at (0, 0) after the reset.
- **Testing**:
  - Verify `ESC c` clears the screen, resets colors, and repositions the cursor, while other functionality (e.g., Tab, arrow keys) remains intact.

### Updated Code
The following C++ code modifies the SDL2 terminal emulator to support the `ESC c` sequence. The `process_ansi_sequence` function is updated to handle `ESC c`, and the parsing logic in the main loop is adjusted to detect it. All other functionality (per-character colors, Tab key, blinking cursor, etc.) remains unchanged.

### Key Changes
1. **ESC c Handling**:
   - Modified `process_ansi_sequence` to accept `final_char` and handle `ESC c`:
     ```cpp
     void process_ansi_sequence(const std::string& seq, char final_char) {
         if (final_char == 'c') { // ESC c: Full reset and clear screen
             // Reset attributes
             current_attr = CharAttr();
             // Reset cursor
             cursor.row = 0;
             cursor.col = 0;
             // Clear screen
             for (int r = 0; r < term_rows; ++r) {
                 for (int c = 0; c < term_cols; ++c) {
                     text_buffer[r][c] = {' ', current_attr};
                 }
                 dirty_lines[r] = true;
                 // Clear texture cache for this line
                 for (auto& span : texture_cache[r]) {
                     if (span.texture) SDL_DestroyTexture(span.texture);
                 }
                 texture_cache[r].clear();
             }
             return;
         }
         // Existing CSI handling...
     }
     ```
   - Resets `current_attr` to default (white foreground, black background).
   - Sets `cursor` to (0, 0).
   - Clears `text_buffer` to spaces with default attributes.
   - Clears `texture_cache` for all lines and marks them dirty.

2. **Parsing Logic**:
   - Updated the `AnsiState::ESCAPE` case in the main loop to check for `c`:
     ```cpp
     case AnsiState::ESCAPE:
         if (c == '[') {
             state = AnsiState::CSI;
             ansi_seq += c;
         } else if (c == 'c') {
             process_ansi_sequence("", c); // Handle ESC c
             state = AnsiState::NORMAL;
             ansi_seq.clear();
         } else {
             state = AnsiState::NORMAL;
             ansi_seq.clear();
         }
         break;
     ```
   - Calls `process_ansi_sequence` with an empty sequence and `c` as the final character.

3. **Preserved Functionality**:
   - Per-character foreground and background colors, Tab key handling, blinking cursor, special keys (arrow, Home, etc.), dynamic resizing, antialiased text, double buffering, and job control remain unchanged.
   - Rendering, input handling, and other ANSI sequences (e.g., SGR, cursor movement) are unaffected.

### How to Compile and Run
#### Prerequisites
- SDL2 and SDL_ttf installed:
  - **Linux**: `sudo apt-get install libsdl2-dev libsdl2-ttf-dev`
  - **macOS**: `brew install sdl2 sdl2_ttf`
- Monospaced font available:
  - Linux: `/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf` (or install `fonts-dejavu`).
  - macOS: `/System/Library/Fonts/Menlo.ttc` (built-in).
  - Adjust the font path in `TTF_OpenFont` if needed.

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
- **ESC c Sequence**:
  - Issuing `echo -e "\033c"` clears the screen, resets colors to white text on black background, and moves the cursor to (0, 0).
  - The blinking cursor appears at the top-left corner, and all text is erased.
  - Subsequent output (e.g., `ls --color`) uses default colors until new ANSI sequences are received.
- **Background/Foreground Colors**:
  - Per-character colors work as before (e.g., `ls --color` shows blue directories on black, green executables).
  - `ESC c` resets all colors, so `echo -e "\033[41mRedBG\033c\033[42mGreenBG"` shows only green background after the reset.
- **Tab Key**:
  - Pressing Tab in `bash` triggers command completion.
  - In `vim`, Tab inserts a tab or spaces.
- **Blinking Cursor**:
  - Blinks (500ms on/off) at the current position, moving to (0, 0) after `ESC c`.
- **Rendering**:
  - Antialiased text with double buffering remains flicker-free.
  - Spans optimize rendering for per-character foreground and background colors.
- **SDL Window**:
  - Opens a window (80x24 characters) with a `/bin/sh` prompt and blinking cursor.
  - Resizing dynamically adjusts the slave terminal size.
- **Dynamic Resizing**:
  - Resizing updates `term_cols` and `term_rows`, and `stty size` reflects the new dimensions.
  - Colors and cursor adjust correctly.
- **Key Input**:
  - Printable characters, Enter, Backspace, Tab, Ctrl+C, Ctrl+D, and special keys work as expected.
- **Slave Output**:
  - Shell output with ANSI sequences (e.g., `ls --color`, `vim`) displays correctly.
- **Job Control**:
  - Job control (`sleep 100 &`, `fg`, `bg`) and signal handling (Ctrl+C) work as before.

### Testing
1. **ESC c Sequence**:
   - Run `echo -e "\033c"` to verify the screen clears, the cursor moves to (0, 0), and colors reset to white text on black.
   - Run `echo -e "\033[41mRedBG\033[32mGreenText\033c"; ls --color` to confirm colors reset and `ls --color` uses default colors until new sequences.
   - In `vim`, type `:r!echo -e "\033c"` and check that the screen clears and the cursor resets.
2. **Background/Foreground Colors**:
   - Run `ls --color` to verify mixed colors (e.g., blue directories, green executables).
   - Test `echo -e "\033[41mRedBG\033[42mGreenBG\033[0m"` for background color changes.
3. **Tab Key**:
   - In `bash`, type `ls` and press Tab to trigger completion.
   - In `vim`, press Tab in insert mode to insert a tab or spaces.
4. **Blinking Cursor**:
   - Verify the cursor blinks at (0, 0) after `echo -e "\033c"`.
   - Ensure it moves correctly with typing, Tab, arrow keys, or `tput cup 5 10`.
5. **Special Keys**:
   - In `vim`, use arrow keys, Home, End, Insert, Delete, Page Up/Down, and F1–F12, ensuring colors and cursor behave correctly.
   - In `less file.txt`, press Page Up/Down to verify scrolling.
6. **Dynamic Resizing**:
   - Resize the window and run `stty size` to confirm dimensions.
   - Verify `vim` adjusts with correct colors after `ESC c`.
7. **Rendering**:
   - Run `vim` or `top` to check flicker-free rendering with per-character colors.
   - Ensure no gaps or overlaps in spans after `ESC c`.
8. **Job Control**:
   - Run `sleep 100 &` and use `fg` or `bg`, confirming colors and cursor behavior.
   - Press Ctrl+Z to suspend and resume.

### Notes
- **Font Path**: Update `TTF_OpenFont` if the font fails to load (e.g., `LiberationMono.ttf` on Linux or another `.ttc` on macOS).
- **ESC c Scope**:
  - `ESC c` resets colors and cursor and clears the screen, as specified. It does not reset terminal modes (e.g., echo, canonical mode) or other states, as these are typically handled by the slave terminal’s `termios` settings. I can add more reset behavior if needed.
- **Performance**:
  - Clearing the texture cache on `ESC c` is efficient, as `dirty_lines` triggers a full redraw only once.
  - Spans optimize rendering for per-character colors.
- **ANSI Support**:
  - Supports `ESC c`, foreground/background colors, cursor movement, and clear sequences. Additional sequences (e.g., alternate screens) can be added if required.
- **Special Keys**:
  - All requested keys (arrow, Home, End, Insert, Delete, Page Up/Down, F1–F12, Tab) are supported. Additional keys (e.g., Alt-modified keys) can be added if needed.

