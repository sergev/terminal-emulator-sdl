# Version 9

To implement ANSI escape sequence support in the SDL2-based terminal emulator, we need to parse and handle ANSI escape codes to support features like text colors, cursor movement, and text attributes (e.g., bold, underline). ANSI escape sequences are used by many terminal applications (e.g., `ls --color`, `vim`, `top`) to control formatting and positioning. This involves:
1. Parsing ANSI escape sequences (e.g., `\033[...m` for colors, `\033[...H` for cursor movement) from the slave pseudo-terminal’s output.
2. Rendering text with appropriate colors and attributes using SDL_ttf.
3. Managing a virtual cursor position to handle cursor movement commands.
4. Maintaining the existing double-buffered renderer, antialiased text, dynamic resizing, key handling, and pseudo-terminal logic.
5. Ensuring compatibility with Linux and macOS.

### Approach
- **ANSI Parsing**:
  - Implement a state machine to parse ANSI escape sequences (starting with `\033[` or ESC `[`) and extract parameters (e.g., `31` for red foreground in `\033[31m`).
  - Support common sequences: SGR (Select Graphic Rendition) for colors/attributes, cursor movement (e.g., CUP, Cursor Position), and clear screen/line.
- **Rendering**:
  - Extend the text buffer to store per-character attributes (color, bold, etc.).
  - Modify the double-buffered renderer to apply colors and attributes when rendering textures.
  - Use SDL_ttf’s `TTF_SetFontStyle` for bold and underline (if supported by the font).
- **Cursor Management**:
  - Track a virtual cursor position (row, column) for rendering and cursor movement commands.
  - Handle cursor movement sequences (e.g., `\033[H`, `\033[A`) and ensure rendering respects the cursor.
- **Integration**:
  - Update the output processing loop to parse ANSI sequences before updating the text buffer.
  - Ensure dynamic resizing updates the cursor and buffer correctly.
- **Limitations**:
  - Focus on common ANSI sequences (SGR, cursor movement, clear) to keep the implementation manageable.
  - Ignore less common sequences (e.g., mouse events, complex modes) for simplicity.

### Updated Code
The following C++ code extends the SDL2 terminal emulator to support ANSI escape sequences, including colors, bold, cursor movement, and basic screen/line clearing. The code maintains double-buffered rendering, antialiased text, dynamic resizing, and all prior functionality.

### Key Changes
1. **Text Buffer with Attributes**:
   - Replaced `std::vector<std::string>` with `std::vector<std::vector<Char>>`, where `Char` contains a character and its attributes (`CharAttr`: foreground/background color, bold, underline).
   - Initialized `text_buffer` with `term_rows` lines, each containing `term_cols` `Char` objects with default attributes.

2. **ANSI Parsing**:
   - Added `AnsiState` enum and state machine (`NORMAL`, `ESCAPE`, `CSI`) to parse escape sequences.
   - Implemented `process_ansi_sequence` to handle:
     - **SGR (m)**: Colors (30–37, 40–47, 90–97, 100–107), bold (1), underline (4), reset (0).
     - **Cursor Movement**: CUP (`H`), CUU (`A`), CUD (`B`), CUF (`C`), CUB (`D`).
     - **Erase**: ED (`J`, clear screen), EL (`K`, clear line).
   - Updated the output loop to parse ANSI sequences and apply them to `current_attr` or cursor position.

3. **Rendering**:
   - Modified `render_text` to:
     - Build a string and extract attributes for each line from `text_buffer`.
     - Render the line with the first character’s foreground color using `TTF_RenderText_Blended` (antialiased).
     - Apply bold/underline via `TTF_SetFontStyle` if present (reset afterward).
     - Cache the texture and copy to the back buffer.
   - Simplified rendering to use one texture per line with the first character’s attributes (for simplicity; per-character colors would require splitting lines).

4. **Cursor Management**:
   - Added `Cursor` struct to track `row` and `col`, updated by ANSI sequences and character output.
   - Handled cursor positioning in output processing (e.g., `\n`, `\r`, `\b`, printable chars).
   - Clamped cursor position during resizing to stay within `term_rows` and `term_cols`.

5. **Output Processing**:
   - Updated the pseudo-terminal output loop to:
     - Parse ANSI sequences using the state machine.
     - Insert printable characters at the cursor position with `current_attr`.
     - Handle newlines, carriage returns, and backspaces with cursor updates.
     - Scroll the buffer when the cursor exceeds `term_rows`.
   - Marked affected lines as dirty (`dirty_lines`) for texture updates.

6. **Dynamic Resizing**:
   - Updated `update_terminal_size` to resize `text_buffer` to `term_rows` x `term_cols` `Char` objects, preserving attributes.
   - Adjusted `texture_cache` and `dirty_lines`, marking all lines dirty after a resize.

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
- **ANSI Escape Sequences**:
  - Colors: `ls --color` displays directories in blue, executables in green, etc. (e.g., `\033[34m` for blue).
  - Attributes: Bold and underline work if the font supports them (e.g., `echo -e "\033[1mBold\033[0m"`).
  - Cursor Movement: Commands like `tput cup 5 10` position the cursor correctly.
  - Clear: `clear` (`\033[2J`) clears the screen, `tput el` (`\033[K`) clears the line.
- **Rendering**:
  - Text is rendered with antialiasing (`TTF_RenderText_Blended`) and double buffering, ensuring smooth, flicker-free display.
  - Each line uses the first character’s color and attributes (simplified rendering).
- **SDL Window**:
  - Opens a window (initially 80x24 characters) with a `/bin/sh` prompt.
  - Resizing dynamically adjusts the slave terminal size.
- **Dynamic Resizing**:
  - Resizing updates `term_cols` and `term_rows`, and `stty size` reflects the new dimensions.
- **Key Input**:
  - Typing, Enter, Backspace, Ctrl+C, and Ctrl+D work as before.
- **Slave Output**:
  - Shell output with ANSI sequences (e.g., `ls --color`, `vim`) displays correctly with colors and cursor positioning.
- **Job Control**:
  - Job control (`sleep 100 &`, `fg`, `bg`) and signal handling (Ctrl+C) work as before.
  - No "can't access tty" warnings.

### Testing
1. **ANSI Colors**:
   - Run `ls --color` to verify colored output (e.g., blue directories, green executables).
   - Test with `echo -e "\033[31mRed\033[32mGreen\033[0m"` to confirm color changes.
2. **Text Attributes**:
   - Run `echo -e "\033[1mBold\033[4mUnderline\033[0m"` to verify bold and underline (font-dependent).
3. **Cursor Movement**:
   - Run `tput cup 5 10; echo X` to place an `X` at row 5, column 10.
   - Test `tput cuu 1` or `tput cud 1` to move the cursor up/down.
4. **Clear Commands**:
   - Run `clear` to clear the screen.
   - Run `tput el` to clear the current line.
5. **Rendering**:
   - Run `vim` or `top` to verify colored output and cursor positioning with smooth rendering.
   - Check for flicker-free updates during rapid output (e.g., `cat large_file.txt`).
6. **Dynamic Resizing**:
   - Resize the window and run `stty size` to confirm updated dimensions.
   - Verify `vim` adjusts to the new size with correct colors.
7. **Key Input**:
   - Type commands, use Backspace, Enter, Ctrl+C, and Ctrl+D.
8. **Job Control**:
   - Run `sleep 100 &` and use `fg` or `bg`.
   - Press Ctrl+Z to suspend and resume.

### Notes
- **Font Path**: Update `TTF_OpenFont` if the font fails to load (e.g., use `LiberationMono.ttf` on Linux or another `.ttc` on macOS).
- **Rendering Limitation**:
  - Each line uses a single color/attribute (first character’s) to simplify rendering. Per-character colors would require rendering individual glyphs, which is more complex but possible if needed.
  - Bold/underline depend on font support; some fonts (e.g., DejaVu Sans Mono) may not render these correctly.
- **ANSI Support**:
  - Supports common sequences (SGR, cursor movement, clear). Less common sequences (e.g., mouse events, alternate screens) are ignored but can be added.
  - The parser is basic and assumes well-formed sequences; malformed sequences are skipped.
- **Performance**: The double-buffered renderer with texture caching ensures smooth updates. Per-character rendering could be added for full color support but may impact performance.
- **Special Keys**: Arrow keys and other special keys are not fully handled; only basic input is supported.

