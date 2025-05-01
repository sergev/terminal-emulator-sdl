# Version 11

To show the cursor position as a blinking rectangle in the SDL2-based terminal emulator, we need to:
1. Render a blinking rectangular cursor at the current cursor position (`cursor.row`, `cursor.col`) in the SDL2 window, using the font's character dimensions (`char_width`, `char_height`) to size and position it.
2. Implement blinking by toggling the cursor's visibility based on a timer (e.g., blinking every 500ms).
3. Integrate the cursor rendering into the existing double-buffered renderer, ensuring it works with ANSI escape sequences, special key support (arrow keys, Home, etc.), antialiased text, dynamic resizing, and pseudo-terminal logic.
4. Ensure compatibility with Linux and macOS, maintaining all prior functionality.

### Approach
- **Cursor Rendering**:
  - Draw a filled rectangle at the cursor’s position (`cursor.col * char_width`, `cursor.row * char_height`) using `SDL_RenderFillRect`.
  - Use a distinct color (e.g., white) to make the cursor visible against the black background and colored text.
- **Blinking Effect**:
  - Track the cursor’s visibility state (`cursor_visible`) and toggle it every 500ms using `SDL_GetTicks` to measure elapsed time.
  - Update the rendering loop to draw the cursor only when `cursor_visible` is true.
- **Integration**:
  - Add cursor rendering to the `render_text` function, after copying cached textures but before presenting the back buffer.
  - Ensure the cursor position is updated correctly by ANSI sequences and special key inputs (e.g., arrow keys, Home).
  - Adjust for dynamic resizing by recalculating the cursor’s pixel position based on the updated `term_cols`, `term_rows`, and font metrics.
- **Preserve Functionality**:
  - Maintain ANSI escape sequence parsing, double-buffered rendering, antialiased text, special key handling, and job control.
  - Ensure the cursor does not interfere with text rendering or terminal behavior.

### Updated Code
The following C++ code extends the previous SDL2 terminal emulator to render a blinking rectangular cursor. The `render_text` function is updated to include cursor rendering, and new variables are added to manage blinking. All other functionality (ANSI support, special keys, dynamic resizing, etc.) remains unchanged.

### Key Changes
1. **Cursor Blinking**:
   - Added global variables:
     - `cursor_visible` (`bool`): Tracks whether the cursor is visible.
     - `last_cursor_toggle` (`Uint32`): Stores the time of the last cursor toggle (in milliseconds).
     - `cursor_blink_interval` (`const Uint32`): Set to 500ms for blinking every 0.5 seconds.
   - In `render_text`, updated the cursor state using `SDL_GetTicks`:
     ```cpp
     Uint32 current_time = SDL_GetTicks();
     if (current_time - last_cursor_toggle >= cursor_blink_interval) {
         cursor_visible = !cursor_visible;
         last_cursor_toggle = current_time;
     }
     ```
   - This toggles `cursor_visible` every 500ms.

2. **Cursor Rendering**:
   - Added cursor rendering in `render_text`, after copying cached textures but before `SDL_RenderPresent`:
     ```cpp
     if (cursor_visible && cursor.row < term_rows && cursor.col < term_cols) {
         SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White cursor
         SDL_Rect cursor_rect = {
             cursor.col * char_width,
             cursor.row * char_height,
             char_width,
             char_height
         };
         SDL_RenderFillRect(renderer, &cursor_rect);
     }
     ```
   - The cursor is a white filled rectangle sized `char_width` x `char_height`, positioned at `(cursor.col * char_width, cursor.row * char_height)`.
   - Only drawn when `cursor_visible` is true and the cursor is within bounds.

3. **No Other Changes**:
   - The rest of the code (ANSI escape sequence parsing, double-buffered rendering, antialiased text, dynamic resizing, special key handling, job control) remains identical.
   - The cursor rendering is a visual addition and does not affect terminal behavior or input/output.

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
- **Blinking Cursor**:
  - A white rectangular cursor appears at the current cursor position (e.g., after the `bash` prompt).
  - The cursor blinks every 500ms (on for 500ms, off for 500ms), creating a standard terminal cursor effect.
  - The cursor moves with input (e.g., typing, arrow keys, Home) and ANSI cursor commands (e.g., `tput cup 5 10`).
- **Rendering**:
  - The cursor is rendered over the text, ensuring visibility against the black background and colored text.
  - Text remains antialiased with double buffering, maintaining flicker-free display with ANSI colors and attributes.
- **SDL Window**:
  - Opens a window (initially 80x24 characters) with a `/bin/sh` prompt and blinking cursor.
  - Resizing dynamically adjusts the slave terminal size, and the cursor scales correctly.
- **Dynamic Resizing**:
  - Resizing updates `term_cols` and `term_rows`, and `stty size` reflects the new dimensions.
  - The cursor remains at the correct position, clamped to the new bounds.
- **Key Input**:
  - Printable characters, Enter, Backspace, Ctrl+C, Ctrl+D, and special keys (arrow, Home, End, etc.) work as before.
  - Arrow keys and cursor movement commands update the cursor’s position, and the blinking rectangle follows.
- **Slave Output**:
  - Shell output with ANSI sequences (e.g., `ls --color`, `vim`) displays with colors, and the cursor blinks at the correct position.
- **Job Control**:
  - Job control (`sleep 100 &`, `fg`, `bg`) and signal handling (Ctrl+C) work as before.
  - No "can't access tty" warnings.

### Testing
1. **Blinking Cursor**:
   - Run the emulator and verify a white rectangular cursor blinks at the `bash` prompt (on/off every 500ms).
   - Type text (e.g., `echo test`) and confirm the cursor moves right, blinking at each new position.
   - Press arrow keys (Up, Down, Left, Right) to move the cursor in `bash` or `vim`, ensuring the blinking rectangle follows.
   - Use `tput cup 5 10; echo X` to position the cursor at row 5, column 10, and verify the rectangle blinks there.
2. **Cursor with Special Keys**:
   - In `vim`, use Home, End, Insert, Delete, Page Up/Down, and F1–F12, checking that the cursor blinks at the updated position.
   - In `less file.txt`, press Page Up/Down and verify the cursor remains visible and blinking.
3. **ANSI Colors**:
   - Run `ls --color` to verify colored output with the blinking cursor visible.
   - Test `echo -e "\033[31mRed\033[0m"` and ensure the cursor blinks after the output.
4. **Dynamic Resizing**:
   - Resize the window and run `stty size` to confirm dimensions.
   - Verify the cursor blinks at the correct position in `vim` or `bash` after resizing.
5. **Rendering**:
   - Run `vim` or `top` to check flicker-free rendering with the blinking cursor.
   - Ensure the cursor is visible over text and does not obscure characters.
6. **Job Control**:
   - Run `sleep 100 &` and use `fg` or `bg`, confirming the cursor blinks correctly.
   - Press Ctrl+Z to suspend and resume, checking cursor behavior.

### Notes
- **Font Path**: Update `TTF_OpenFont` if the font fails to load (e.g., `LiberationMono.ttf` on Linux or another `.ttc` on macOS).
- **Cursor Appearance**:
  - The cursor is a solid white rectangle for visibility. If you prefer a different style (e.g., underscore, vertical bar, or different color), I can adjust it.
  - Blinking at 500ms matches common terminal behavior; I can change the interval if desired.
- **Rendering Limitation**:
  - Each line uses a single color/attribute (first character’s). Per-character colors require a more complex renderer, which I can implement if needed.
- **Performance**: The cursor rendering adds minimal overhead, and double buffering ensures flicker-free updates.
- **Special Keys**: All requested keys (arrow, Home, End, Insert, Delete, Page Up/Down, F1–F12) are supported. Additional keys (e.g., Tab, Alt-modified keys) can be added if required.

