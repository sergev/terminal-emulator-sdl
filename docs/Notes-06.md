# Version 6

To enhance the terminal emulator by placing it into a text window using the SDL2 library, we need to:
1. Create an SDL2 window and renderer to display text.
2. Use a monospaced font (via SDL_ttf) to render characters from the slave pseudo-terminal.
3. Handle SDL2 key press events, convert them to characters, and send them to the slave pseudo-terminal.
4. Read characters from the slave pseudo-terminal and render them in the SDL2 window.
5. Maintain the existing pseudo-terminal logic (job control, window size handling, signal forwarding) from the previous version.
6. Ensure compatibility with both Linux and macOS.

### Approach
- **SDL2 Setup**: Initialize SDL2 and SDL_ttf, create a window, and load a monospaced font (e.g., DejaVu Sans Mono).
- **Text Rendering**: Use a simple text buffer to store the terminal content and render it line-by-line in the SDL2 window.
- **Key Handling**: Capture SDL2 key press events, map them to characters (handling printable keys and special keys like Enter), and write to the master pseudo-terminal.
- **Slave Output**: Read from the master pseudo-terminal (non-blocking) and append characters to the text buffer, updating the SDL2 window.
- **Pseudo-Terminal Integration**: Fork the `/bin/sh` process as before, using the existing pseudo-terminal setup.
- **Window Size**: Update the slave terminal size (`TIOCSWINSZ`) based on the SDL2 window size (in character units, calculated from font metrics).

### Assumptions
- SDL2 and SDL_ttf libraries are installed:
  - On Linux: `sudo apt-get install libsdl2-dev libsdl2-ttf-dev`
  - On macOS: `brew install sdl2 sdl2_ttf`
- A monospaced font (e.g., DejaVu Sans Mono) is available. You can replace it with another font (e.g., `/System/Library/Fonts/Menlo.ttc` on macOS or `/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf` on Linux).
- The terminal supports basic ASCII/UTF-8 characters; advanced ANSI escape sequences (e.g., colors, cursor movement) are not handled in this version to keep it simple.

### Updated Code
The following C++ code integrates the terminal emulator into an SDL2 window, handling key presses and slave output while preserving the previous functionality. It uses SDL2 for the window and SDL_ttf for text rendering, and it remains compatible with Linux and macOS.

### Key Changes
1. **SDL2 Integration**:
   - Initialized SDL2 (`SDL_Init`) and SDL_ttf (`TTF_Init`) to create a window and load a monospaced font.
   - Created an SDL window (`max_cols * char_width` x `max_rows * char_height`) and renderer.
   - Loaded a font with fallback paths for Linux (`DejaVuSansMono.ttf`) and macOS (`Menlo.ttc`).

2. **Text Rendering**:
   - Used a `text_buffer` (vector of strings) to store terminal lines, with a maximum of `max_rows` lines and `max_cols` characters per line.
   - Implemented `render_text` to clear the SDL window, render each line using `TTF_RenderText_Solid`, and display it at the correct position.
   - Handled basic character processing (printable ASCII, newline, carriage return, backspace).

3. **Key Handling**:
   - Processed SDL key events (`SDL_KEYDOWN`):
     - Printable characters (ASCII 32–126) are sent to `master_fd`.
     - Enter sends `\n`, Backspace sends `\b`.
     - Ctrl+C sends `SIGINT` to the child, Ctrl+D exits the emulator.
   - Ignored unsupported keys for simplicity.

4. **Slave Output**:
   - Set `master_fd` to non-blocking mode (`O_NONBLOCK`) to avoid blocking in the main loop.
   - Used `select` with a 10ms timeout to check for slave output.
   - Processed output characters:
     - Newline (`\n`) starts a new line.
     - Carriage return (`\r`) clears the current line or handles `\r\n`.
     - Backspace (`\b`) removes the last character.
     - Printable characters are appended to the current line.
   - Limited the buffer to `max_rows` and each line to `max_cols`.

5. **Window Size Handling**:
   - Set the slave terminal size to `max_cols` x `max_rows` (80x24) at startup, using font metrics for pixel dimensions.
   - Updated `handle_sigwinch` to use the fixed `max_cols` and `max_rows`, triggered on `SDL_WINDOWEVENT_RESIZED`.
   - Sent `SIGWINCH` to the child process to notify the shell of size changes.

6. **Pseudo-Terminal Logic**:
   - Retained the existing pseudo-terminal setup (`posix_openpt`, `fork`, `/bin/sh`, job control, signal forwarding).
   - Removed `set_raw_terminal` and `restore_terminal`, as the SDL window handles input/output instead of `stdin`/`stdout`.

### How to Compile and Run
#### Prerequisites
- Install SDL2 and SDL_ttf:
  - **Linux**: `sudo apt-get install libsdl2-dev libsdl2-ttf-dev`
  - **macOS**: `brew install sdl2 sdl2_ttf`
- Ensure a monospaced font is available:
  - Linux: `/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf` (or install `fonts-dejavu`).
  - macOS: `/System/Library/Fonts/Menlo.ttc` (built-in).
  - Adjust the font path in the code if necessary.

#### On macOS
1. Save the code as `terminal_emulator_sdl.cpp`.
2. Compile with `clang++`, linking SDL2 and SDL_ttf:
   ```bash
   clang++ -o terminal_emulator_sdl terminal_emulator_sdl.cpp -I/usr/local/include -L/usr/local/lib -lSDL2 -lSDL2_ttf
   ```
   (Adjust include/library paths if Homebrew installed SDL2 elsewhere, e.g., `/opt/homebrew`.)
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
  - Opens a window (80x24 characters, sized by font metrics) displaying a `/bin/sh` prompt.
  - Text is white on a black background, using a monospaced font.
- **Key Input**:
  - Typing printable characters (e.g., `a`, `1`, `@`) sends them to the shell, displayed in the window.
  - Enter sends a newline, Backspace deletes the last character.
  - Ctrl+C interrupts the current command, Ctrl+D exits the emulator.
- **Slave Output**:
  - Shell output (e.g., from `ls`, `echo`) appears in the window, with newlines creating new lines.
  - The buffer scrolls up when exceeding 24 lines.
- **Window Size**:
  - The slave terminal is fixed at 80x24 characters, updated via `SIGWINCH` when the SDL window is resized.
  - Commands like `stty size` report `24 80`.
- **Job Control**:
  - Shell job control works (e.g., `sleep 100 &`, `fg`, `bg`).
  - No "can't access tty" warnings.

### Testing
1. **Key Input**:
   - Type `ls` and press Enter to see directory output.
   - Use Backspace to correct input.
   - Press Ctrl+C to interrupt a command like `sleep 10`.
2. **Slave Output**:
   - Run `echo Hello` or `cat` to verify output displays correctly.
   - Test newlines with `printf "A\nB\nC\n"`.
3. **Window Size**:
   - Resize the SDL window and run `stty size` to confirm `24 80`.
   - Open `vim` to verify it uses the full 80x24 area.
4. **Job Control**:
   - Run `sleep 100 &` and use `fg` or `bg`.
   - Press Ctrl+Z to suspend a command and resume it.

### Notes
- **Font Path**: If the font fails to load, replace the path in `TTF_OpenFont` with a valid monospaced font on your system (e.g., `LiberationMono.ttf` on Linux or another `.ttc` file on macOS).
- **Limitations**:
  - Only basic ASCII characters are handled; ANSI escape sequences (e.g., colors, cursor movement) are not processed. If you need this, I can extend the code.
  - The terminal size is fixed at 80x24 for simplicity. Dynamic resizing based on window size can be added if needed.
  - Special keys (e.g., arrow keys, Tab) are not fully handled; only basic input is supported.
- **Performance**: The text rendering is simple and may flicker on rapid updates. For production use, consider double-buffering or texture caching.
- **Cleanup**: The program terminates the child process with `SIGTERM` and cleans up SDL2 resources on exit.

