# Version 13

To implement per-character background color support and Tab key handling in the SDL2-based terminal emulator, we need to:
1. **Background Colors**:
   - Modify the rendering system to draw background colors for each character span, using the `CharAttr::bg` field (SDL_Color) stored in the text buffer.
   - Render a filled rectangle for each span’s background before drawing its text texture, ensuring the background aligns with the text.
   - Remove bold and underline support (as requested), simplifying `CharAttr` and rendering logic.
2. **Tab Key**:
   - Capture the Tab key (`SDLK_TAB`) in the SDL2 key event handler and send the appropriate control code (`\t`, ASCII 0x09) to the slave pseudo-terminal.
   - Ensure Tab integrates with existing key handling (arrow keys, Home, etc.) and works in applications like `bash` (e.g., for command completion) and `vim`.
3. **Preserve Functionality**:
   - Maintain per-character foreground color support, blinking cursor, ANSI escape sequence parsing, special key support, dynamic resizing, antialiased text, double-buffered rendering, and job control.
   - Ensure compatibility with Linux and macOS.

### Approach
- **Background Colors**:
  - Update `CharAttr` to store only foreground (`fg`) and background (`bg`) colors, removing `bold` and `underline`.
  - In `render_text`, for each `TextSpan`, draw a background rectangle using `SDL_RenderFillRect` with the span’s `bg` color, sized to cover the span’s text (width = `text.length() * char_width`, height = `char_height`).
  - Draw the span’s text texture (with `fg` color) over the background rectangle.
  - Update ANSI parsing to handle background color codes (40–47, 100–107) without bold/underline (1, 4).
- **Tab Key**:
  - Add `SDLK_TAB` to the `SDL_KEYDOWN` handler, mapping it to `\t`.
  - Ensure Tab works in `bash` (triggers completion) and `vim` (inserts tab or spaces, depending on settings).
- **Rendering Optimization**:
  - Continue using spans to group characters with identical attributes (`fg` and `bg`), minimizing the number of textures and rectangles.
  - Cache span textures to avoid redundant rendering, using `dirty_lines` to track changes.
- **Integration**:
  - Preserve the blinking cursor (white rectangle, 500ms interval).
  - Ensure dynamic resizing updates background rectangles and texture cache correctly.
  - Maintain all other functionality (ANSI sequences, special keys, job control).

### Updated Code
The following C++ code modifies the SDL2 terminal emulator to support per-character background colors, handle the Tab key, and remove bold/underline support. The `CharAttr` struct, `render_text` function, ANSI parsing, and key handling are updated, while other functionality remains intact.

### Key Changes
1. **Background Color Support**:
   - Updated `CharAttr` to remove `bold` and `underline`, keeping only `fg` and `bg`:
     ```cpp
     struct CharAttr {
         SDL_Color fg = {255, 255, 255, 255};
         SDL_Color bg = {0, 0, 0, 255};
         bool operator==(const CharAttr& other) const { ... }
     };
     ```
   - Modified `render_text` to draw a background rectangle for each span before its text:
     ```cpp
     SDL_SetRenderDrawColor(renderer, span.attr.bg.r, span.attr.bg.g, span.attr.bg.b, span.attr.bg.a);
     SDL_Rect bg_rect = {
         span.start_col * char_width,
         static_cast<int>(i * char_height),
         static_cast<int>(span.text.length() * char_width),
         char_height
     };
     SDL_RenderFillRect(renderer, &bg_rect);
     ```
   - The rectangle’s width is `span.text.length() * char_width` to match the text’s width, ensuring alignment.
   - Updated `render_text` to remove bold/underline rendering (no `TTF_SetFontStyle` calls).

2. **ANSI Parsing**:
   - Updated `process_ansi_sequence` to ignore bold (1) and underline (4) codes, handling only:
     - Foreground colors: 30–37, 90–97
     - Background colors: 40–47, 100–107
     - Reset: 0
   - Example:
     ```cpp
     if (p == 0) { // Reset
         current_attr = CharAttr();
     } else if (p >= 30 && p <= 37) { // Foreground color
         current_attr.fg = ansi_colors[p - 30];
     } else if (p >= 40 && p <= 47) { // Background color
         current_attr.bg = ansi_colors[p - 40];
     } ...
     ```

3. **Tab Key Handling**:
   - Added `SDLK_TAB` to the `SDL_KEYDOWN` handler in the main loop:
     ```cpp
     case SDLK_TAB:
         input = "\t";
         break;
     ```
   - Sends `\t` (ASCII 0x09) to `master_fd` via `write`, enabling command completion in `bash` or tab insertion in `vim`.

4. **Preserved Functionality**:
   - Per-character foreground colors are maintained, with spans grouping characters by both `fg` and `bg`.
   - The blinking cursor (white rectangle, 500ms interval) remains unchanged.
   - All other functionality (ANSI sequences, special keys, dynamic resizing, double buffering, job control) is preserved.
   - Texture caching and `dirty_lines` ensure efficient rendering.

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
- **Background Colors**:
  - Each character span can have its own foreground and background color, as specified by ANSI escape sequences (e.g., `\033[41m` for red background, `\033[34m` for blue foreground).
  - Applications like `vim` (syntax highlighting) or `ls --color` display mixed foreground and background colors within a line.
  - Background rectangles align perfectly with text, creating a seamless terminal appearance.
- **Tab Key**:
  - Pressing Tab in `bash` triggers command completion (e.g., type `ls` and press Tab to list matching commands).
  - In `vim`, Tab inserts a tab character or spaces (depending on `:set expandtab`).
- **Blinking Cursor**:
  - A white rectangular cursor blinks (500ms on/off) at the current position, visible over colored text and backgrounds.
  - Moves correctly with input, arrow keys, Tab, and ANSI cursor commands (e.g., `tput cup 5 10`).
- **Rendering**:
  - Text is antialiased (`TTF_RenderText_Blended`) and rendered flicker-free with double buffering.
  - Spans optimize rendering by grouping characters with the same `fg` and `bg`.
- **SDL Window**:
  - Opens a window (initially 80x24 characters) with a `/bin/sh` prompt and blinking cursor.
  - Resizing dynamically adjusts the slave terminal size.
- **Dynamic Resizing**:
  - Resizing updates `term_cols` and `term_rows`, and `stty size` reflects the new dimensions.
  - Foreground/background colors and the cursor adjust correctly.
- **Key Input**:
  - Printable characters, Enter, Backspace, Tab, Ctrl+C, Ctrl+D, and special keys (arrow, Home, End, etc.) work as expected.
- **Slave Output**:
  - Shell output with ANSI sequences (e.g., `ls --color`, `vim`) displays with per-character foreground and background colors.
- **Job Control**:
  - Job control (`sleep 100 &`, `fg`, `bg`) and signal handling (Ctrl+C) work as before.
  - No "can't access tty" warnings.

### Testing
1. **Background Colors**:
   - Run `ls --color` to verify mixed foreground/background colors (e.g., white text on blue for directories, green text on black for executables).
   - In `vim` with syntax highlighting (e.g., `vim test.py`), confirm keywords, strings, and comments use different foreground and background colors on the same line.
   - Test with `echo -e "\033[41mRedBG\033[42mGreenBG\033[0m"` to see red and green backgrounds in one line.
2. **Tab Key**:
   - In `bash`, type `ls` and press Tab to trigger completion (e.g., lists `ls`, `lsblk`).
   - In `vim`, press Tab in insert mode to insert a tab or spaces, and verify the cursor moves correctly.
   - Run `cat > test.txt`, press Tab, and type text to confirm tab characters are inserted.
3. **Blinking Cursor**:
   - Verify the white cursor blinks at the `bash` prompt and moves with typing, Tab, arrow keys, or `tput cup 5 10`.
   - Ensure the cursor is visible over colored text and backgrounds in `vim` or `ls --color`.
4. **Removed Attributes**:
   - Test `echo -e "\033[1mBold\033[4mUnderline\033[0m"` to confirm bold/underline are ignored (text remains normal).
5. **Special Keys**:
   - In `vim`, use arrow keys, Home, End, Insert, Delete, Page Up/Down, and F1–F12, ensuring colors and cursor behave correctly.
   - In `less file.txt`, press Page Up/Down and verify scrolling with colored output.
6. **Dynamic Resizing**:
   - Resize the window and run `stty size` to confirm dimensions.
   - Verify `vim` adjusts with per-character foreground/background colors and a blinking cursor.
7. **Rendering**:
   - Run `vim` or `top` to check flicker-free rendering with per-character colors.
   - Ensure no gaps or overlaps between spans, and backgrounds align with text.
8. **Job Control**:
   - Run `sleep 100 &` and use `fg` or `bg`, confirming colors and cursor behavior.
   - Press Ctrl+Z to suspend and resume.

### Notes
- **Font Path**: Update `TTF_OpenFont` if the font fails to load (e.g., `LiberationMono.ttf` on Linux or another `.ttc` on macOS).
- **Performance**:
  - Rendering background rectangles adds slight GPU overhead, but spans minimize the number of draw calls.
  - For very wide lines with frequent color changes, performance may degrade slightly. A glyph atlas could optimize this if needed.
- **Background Colors**:
  - Background colors are rendered per span, matching the text’s position and size.
  - If the background color matches the terminal’s black background (`0, 0, 0, 255`), it blends seamlessly.
- **Tab Handling**:
  - Tab sends `\t`, which `bash` interprets as completion and `vim` as indentation. Some applications may handle Tab differently (e.g., `zsh`); I can adjust if needed.
- **ANSI Support**:
  - Supports foreground (30–37, 90–97) and background (40–47, 100–107) colors, cursor movement, and clear sequences. Additional sequences (e.g., alternate screens) can be added if required.
- **Special Keys**:
  - All requested keys (arrow, Home, End, Insert, Delete, Page Up/Down, F1–F12, Tab) are supported. Additional keys (e.g., Alt-modified keys) can be added if needed.

