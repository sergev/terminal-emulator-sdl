# Version 8

To implement a double-buffered renderer in the SDL2-based terminal emulator, we need to optimize the rendering process to reduce flickering and improve performance. Double buffering involves maintaining two buffers: one for rendering (back buffer) and one for display (front buffer). SDL2 inherently supports double buffering when using a renderer with `SDL_RENDERER_ACCELERATED`, but we can enhance the rendering pipeline by:
1. Pre-rendering text textures for each line in the text buffer and caching them to avoid redundant rendering.
2. Using a separate rendering pass to update the back buffer, then swapping it with the front buffer using `SDL_RenderPresent`.
3. Ensuring the double-buffered approach integrates with the existing antialiased text rendering (`TTF_RenderText_Blended`), dynamic resizing, key handling, and pseudo-terminal logic.
4. Maintaining compatibility with Linux and macOS.

### Approach
- **Texture Caching**: Create a cache of SDL textures for each line in the `text_buffer`, updating only when the line’s content changes or the terminal size changes.
- **Double-Buffered Rendering**: Modify `render_text` to:
  - Clear the back buffer.
  - Copy cached textures to the back buffer.
  - Present the back buffer to the screen (swapping with the front buffer).
- **Buffer Management**: Track dirty lines (those that have changed) to update only necessary textures, minimizing rendering overhead.
- **Integration**: Ensure the double-buffered renderer works with dynamic resizing (updating textures on size changes) and maintains antialiased text quality.
- **Performance**: Reduce CPU/GPU load by avoiding redundant `TTF_RenderText_Blended` calls and leveraging SDL2’s hardware-accelerated rendering.

### Updated Code
The following C++ code modifies the SDL2 terminal emulator to implement a double-buffered renderer with texture caching. The `render_text` function is updated, and a texture cache is added, while all other functionality (antialiased text, dynamic resizing, key handling, pseudo-terminal logic) remains unchanged.

### Key Changes
1. **Texture Cache**:
   - Added `texture_cache` (vector of `SDL_Texture*`) to store pre-rendered textures for each line in `text_buffer`.
   - Added `dirty_lines` (vector of `bool`) to track which lines have changed and need their textures updated.
   - Initialized `texture_cache` and `dirty_lines` with the text buffer (one empty line, marked dirty).

2. **Double-Buffered Rendering**:
   - Updated `render_text` to:
     - Update textures only for dirty lines using `TTF_RenderText_Blended` (antialiased).
     - Destroy old textures before creating new ones to avoid memory leaks.
     - Clear the back buffer with a black background.
     - Copy cached textures to the back buffer using `SDL_RenderCopy`.
     - Present the back buffer with `SDL_RenderPresent`, swapping it with the front buffer.
   - Marked lines as clean (`dirty_lines[i] = false`) after updating their textures.

3. **Text Buffer Updates**:
   - Modified the pseudo-terminal output loop to mark lines as dirty (`dirty_lines.back() = true`) when characters are added, removed, or cleared (e.g., `\n`, `\r`, `\b`, printable chars).
   - Ensured `texture_cache` and `dirty_lines` stay synchronized with `text_buffer` during insertions (e.g., newlines) and deletions (e.g., trimming on resize).

4. **Dynamic Resizing**:
   - Updated `update_terminal_size` to resize `texture_cache` and `dirty_lines` alongside `text_buffer`, marking all lines dirty to force texture updates after a resize.
   - Destroyed textures for removed lines to prevent memory leaks.

5. **Cleanup**:
   - Added cleanup for `texture_cache` in the main loop’s exit path, destroying all textures to free resources.

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
- **Double-Buffered Rendering**:
  - Text rendering is flicker-free, even during rapid updates (e.g., scrolling output from `cat` or `ls -R`).
  - Textures are cached and reused for unchanged lines, reducing rendering overhead.
  - Antialiased text (via `TTF_RenderText_Blended`) remains smooth and high-quality.
- **SDL Window**:
  - Opens a window (initially 80x24 characters) with a `/bin/sh` prompt, using white antialiased text on a black background.
  - Resizing the window dynamically adjusts the slave terminal size.
- **Dynamic Resizing**:
  - Resizing updates `term_cols` and `term_rows`, and textures are regenerated for all lines (marked dirty).
  - Commands like `stty size` reflect the new dimensions (e.g., `30 100`).
- **Key Input**:
  - Typing, Enter, Backspace, Ctrl+C, and Ctrl+D work as before.
- **Slave Output**:
  - Shell output (e.g., `ls`, `echo`) displays correctly with smooth, flicker-free rendering.
- **Job Control**:
  - Job control (e.g., `sleep 100 &`, `fg`, `bg`) and signal handling (Ctrl+C) work as before.
  - No "can't access tty" warnings.

### Testing
1. **Double-Buffered Rendering**:
   - Run `ls -R` or `cat large_file.txt` to verify that output scrolls without flickering.
   - Type rapidly (e.g., hold a key) to ensure input updates are smooth.
   - Compare with the previous version to confirm reduced or eliminated flickering.
2. **Antialiased Text**:
   - Verify that text remains smooth and antialiased (no jagged edges).
   - Test with larger font sizes (e.g., change `TTF_OpenFont` size to 24) to ensure quality.
3. **Dynamic Resizing**:
   - Resize the SDL window and run `stty size` to confirm updated dimensions (e.g., `30 100`).
   - Open `vim` or `top` and resize to verify they adjust with smooth rendering.
4. **Text Rendering**:
   - Run `printf "A\nB\nC\n"` or `cat` to ensure output wraps and scrolls correctly.
5. **Key Input**:
   - Type commands, use Backspace, Enter, Ctrl+C, and Ctrl+D to confirm functionality.
6. **Job Control**:
   - Run `sleep 100 &` and use `fg` or `bg`.
   - Press Ctrl+Z to suspend and resume commands.

### Notes
- **Font Path**: If the font fails to load, update the `TTF_OpenFont` path to a valid monospaced font (e.g., `LiberationMono.ttf` on Linux or another `.ttc` on macOS).
- **Performance**: The double-buffered renderer with texture caching significantly reduces flickering and improves efficiency by avoiding redundant text rendering. For even higher performance, we could cache textures across frames more aggressively or use a glyph atlas.
- **Limitations**:
  - Still handles only basic ASCII; ANSI escape sequences (e.g., colors, cursor movement) are not supported. I can add this if needed.
  - Special keys (e.g., arrow keys, Tab) are not fully handled; only basic input is supported.
- **Debug Output**: Check `std::cerr` for resize events or errors (e.g., `ioctl` failures).

