# Version 12

To implement per-character color support in the SDL2-based terminal emulator, we need to modify the rendering system to handle individual character colors and attributes, as specified by ANSI escape sequences. Currently, each line is rendered with a single color and attribute (based on the first character), which limits the emulator's ability to display text with varying colors within a line (e.g., in `vim` syntax highlighting or `ls --color` with mixed colors). This involves:
1. Rendering each character (or span of characters with the same attributes) separately, using SDL_ttf to create textures for individual characters or segments.
2. Updating the double-buffered renderer to composite these per-character textures onto the back buffer.
3. Preserving the blinking cursor, ANSI escape sequence parsing, special key support (arrow keys, Home, etc.), dynamic resizing, antialiased text, and pseudo-terminal logic.
4. Ensuring compatibility with Linux and macOS, maintaining all prior functionality.

### Approach
- **Text Buffer**: Continue using `std::vector<std::vector<Char>>`, where `Char` contains a character and its attributes (`CharAttr`: foreground/background color, bold, underline). This already stores per-character attributes.
- **Rendering**:
  - Instead of rendering each line as a single texture, iterate through each character in a line.
  - Group consecutive characters with identical attributes into "spans" to optimize rendering (avoiding one texture per character).
  - Create a texture for each span using `TTF_RenderText_Blended`, applying the span’s foreground color, bold, and underline attributes.
  - Position each span’s texture at the correct pixel coordinates (`col * char_width`, `row * char_height`).
- **Texture Caching**:
  - Cache textures for each line, but store a vector of textures per line (one per span) instead of a single texture.
  - Update the cache only when a line’s content or attributes change (using `dirty_lines`).
- **Cursor Rendering**:
  - Keep the blinking cursor as a white rectangle, ensuring it renders correctly over per-character colored text.
- **Integration**:
  - Update `render_text` to handle per-character rendering while maintaining double buffering.
  - Ensure dynamic resizing adjusts the texture cache and cursor position correctly.
  - Preserve ANSI parsing, special key handling, and job control.

### Challenges
- **Performance**: Rendering multiple textures per line increases GPU load compared to one texture per line. Using spans minimizes this by grouping characters with the same attributes.
- **Alignment**: Ensure precise positioning of character textures to avoid gaps or overlaps, using `char_width` and `char_height`.
- **Memory**: Manage texture memory carefully, destroying old textures when lines change or the terminal resizes.

### Updated Code
The following C++ code modifies the SDL2 terminal emulator to support per-character colors by rendering spans of characters with the same attributes. The `render_text` function and texture cache are updated, while all other functionality (blinking cursor, ANSI sequences, special keys, dynamic resizing, etc.) remains intact.

### Key Changes
1. **TextSpan Structure**:
   - Added `TextSpan` struct to represent a span of characters with the same attributes:
     ```cpp
     struct TextSpan {
         std::string text;
         CharAttr attr;
         int start_col;
         SDL_Texture* texture = nullptr;
     };
     ```
   - Stores the text, attributes, starting column, and texture for each span.

2. **Texture Cache Update**:
   - Changed `texture_cache` from `std::vector<SDL_Texture*>` to `std::vector<std::vector<TextSpan>>`, where each line contains a list of spans.
   - Initialized `texture_cache` with `term_rows` empty vectors.
   - Updated `update_terminal_size` and output processing to manage `texture_cache` as a vector of span lists.

3. **Per-Character Color Rendering**:
   - Modified `render_text` to:
     - For each dirty line, clear existing textures and build new spans by grouping characters with identical attributes.
     - Compare attributes using a custom `operator==` in `CharAttr` to check foreground/background color, bold, and underline.
     - Render each span with `TTF_RenderText_Blended`, applying the span’s foreground color and bold/underline styles.
     - Store the texture and metadata in `texture_cache[i]`.
     - Copy each span’s texture to the back buffer at `(span.start_col * char_width, i * char_height)`.
   - Example rendering loop:
     ```cpp
     for (const auto& span : texture_cache[i]) {
         if (!span.texture) continue;
         int w, h;
         SDL_QueryTexture(span.texture, nullptr, nullptr, &w, &h);
         SDL_Rect dst = {span.start_col * char_width, static_cast<int>(i * char_height), w, h};
         SDL_RenderCopy(renderer, span.texture, nullptr, &dst);
     }
     ```

4. **Memory Management**:
   - Destroyed old textures when a line is dirty or during resizing to prevent memory leaks.
   - Ensured cleanup of all textures in the exit path:
     ```cpp
     for (auto& line_spans : texture_cache) {
         for (auto& span : line_spans) {
             if (span.texture) SDL_DestroyTexture(span.texture);
         }
     }
     ```

5. **Preserved Functionality**:
   - The blinking cursor remains a white rectangle, rendered after text to ensure visibility.
   - ANSI escape sequence parsing, special key handling (arrow keys, Home, etc.), dynamic resizing, and job control are unchanged.
   - Antialiased text and double buffering are maintained, with spans ensuring smooth rendering.

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
- **Per-Character Colors**:
  - Each character can have its own foreground color, as specified by ANSI escape sequences (e.g., `\033[31m` for red, `\033[32m` for green).
  - Applications like `vim` (with syntax highlighting) or `ls --color` display mixed colors within a line (e.g., blue directories, green executables).
  - Bold and underline attributes are applied per span, if supported by the font.
- **Blinking Cursor**:
  - A white rectangular cursor blinks (500ms on/off) at the current position, visible over colored text.
  - Moves correctly with input, arrow keys, and ANSI cursor commands (e.g., `tput cup 5 10`).
- **Rendering**:
  - Text is antialiased (`TTF_RenderText_Blended`) and rendered flicker-free with double buffering.
  - Spans ensure efficient rendering by grouping characters with the same attributes.
- **SDL Window**:
  - Opens a window (initially 80x24 characters) with a `/bin/sh` prompt and blinking cursor.
  - Resizing dynamically adjusts the slave terminal size.
- **Dynamic Resizing**:
  - Resizing updates `term_cols` and `term_rows`, and `stty size` reflects the new dimensions.
  - Per-character colors and the cursor adjust correctly.
- **Key Input**:
  - Printable characters, Enter, Backspace, Ctrl+C, Ctrl+D, and special keys (arrow, Home, End, etc.) work as before.
- **Slave Output**:
  - Shell output with ANSI sequences (e.g., `ls --color`, `vim`) displays with per-character colors and correct cursor positioning.
- **Job Control**:
  - Job control (`sleep 100 &`, `fg`, `bg`) and signal handling (Ctrl+C) work as before.
  - No "can't access tty" warnings.

### Testing
1. **Per-Character Colors**:
   - Run `ls --color` to verify mixed colors (e.g., blue directories, green executables, red archives) within a single line.
   - In `vim` with syntax highlighting (e.g., `vim test.py`), confirm keywords, strings, and comments use different colors on the same line.
   - Test with `echo -e "\033[31mRed\033[32mGreen\033[34mBlue\033[0m"` to see multiple colors in one line.
2. **Blinking Cursor**:
   - Verify the white cursor blinks at the `bash` prompt and moves with typing, arrow keys, or `tput cup 5 10`.
   - Ensure the cursor is visible over colored text in `vim` or `ls --color`.
3. **Text Attributes**:
   - Run `echo -e "\033[1mBold\033[4mUnderline\033[0m"` to check bold/underline (font-dependent) with per-character colors.
4. **Special Keys**:
   - In `vim`, use arrow keys, Home, End, Insert, Delete, Page Up/Down, and F1–F12, ensuring colors and cursor behave correctly.
   - In `less file.txt`, press Page Up/Down and verify scrolling with colored output.
5. **Dynamic Resizing**:
   - Resize the window and run `stty size` to confirm dimensions.
   - Verify `vim` adjusts with per-character colors and a blinking cursor.
6. **Rendering**:
   - Run `vim` or `top` to check flicker-free rendering with per-character colors.
   - Ensure no gaps or overlaps between character spans.
7. **Job Control**:
   - Run `sleep 100 &` and use `fg` or `bg`, confirming colors and cursor behavior.
   - Press Ctrl+Z to suspend and resume.

### Notes
- **Font Path**: Update `TTF_OpenFont` if the font fails to load (e.g., `LiberationMono.ttf` on Linux or another `.ttc` on macOS).
- **Performance**:
  - Rendering spans increases GPU load but is optimized by grouping characters with the same attributes.
  - For very wide lines with frequent color changes, performance may degrade slightly. A glyph atlas could further optimize this if needed.
- **Rendering Details**:
  - Background colors (`CharAttr::bg`) are not rendered (only foreground colors are used). I can add background color support if desired, though it requires rendering rectangles behind each span.
  - Bold/underline depend on font support; some fonts (e.g., DejaVu Sans Mono) may not render these correctly.
- **ANSI Support**: Supports common sequences (SGR, cursor movement, clear). Additional sequences (e.g., alternate screens) can be added if needed.
- **Special Keys**: All requested keys (arrow, Home, End, Insert, Delete, Page Up/Down, F1–F12) are supported. Additional keys (e.g., Tab) can be added if required.

