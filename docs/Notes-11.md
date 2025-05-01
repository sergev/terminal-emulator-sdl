# Version 10

To implement support for arrow keys, Home, End, Insert, Delete, Page Up, Page Down, and F1–F12 keys in the SDL2-based terminal emulator, we need to:
1. Capture SDL2 key events for these special keys and map them to the appropriate ANSI escape sequences or control codes expected by the shell (`/bin/sh`).
2. Send these sequences to the slave pseudo-terminal (`master_fd`) to allow applications like `bash`, `vim`, or `less` to interpret them correctly.
3. Ensure the existing functionality (ANSI escape sequence parsing, double-buffered rendering, antialiased text, dynamic resizing, job control, and Linux/macOS compatibility) remains intact.

### Approach
- **Key Mapping**:
  - Arrow keys (Up, Down, Left, Right): Map to ANSI escape sequences (e.g., `\033[A` for Up).
  - Home, End, Insert, Delete: Map to standard terminal sequences (e.g., `\033[H` for Home, `\033[P` for Delete).
  - Page Up, Page Down: Map to sequences like `\033[5~` and `\033[6~`.
  - F1–F12: Map to sequences like `\033[11~` to `\033[23~` (varies by terminal type).
- **SDL2 Key Handling**:
  - Update the `SDL_KEYDOWN` event handler to detect these keys using `SDLK_` constants (e.g., `SDLK_UP`, `SDLK_HOME`, `SDLK_F1`).
  - Send the corresponding escape sequences to `master_fd` using `write`.
- **Integration**:
  - Preserve existing key handling (printable characters, Enter, Backspace, Ctrl+C, Ctrl+D).
  - Ensure ANSI parsing, rendering, and cursor management are unaffected.
- **Terminal Compatibility**:
  - Use sequences compatible with common terminals (e.g., xterm, vt100) to ensure `bash`, `vim`, and other applications interpret them correctly.
  - Account for slight variations in sequence interpretation across Linux and macOS shells.

### Key Mappings
The following mappings are based on standard xterm/vt100 escape sequences, which are widely supported by shells and applications:
- **Arrow Keys**:
  - Up: `\033[A`
  - Down: `\033[B`
  - Right: `\033[C`
  - Left: `\033[D`
- **Home/End**:
  - Home: `\033[H`
  - End: `\033[F`
- **Insert/Delete**:
  - Insert: `\033[2~`
  - Delete: `\033[3~`
- **Page Up/Page Down**:
  - Page Up: `\033[5~`
  - Page Down: `\033[6~`
- **Function Keys (F1–F12)**:
  - F1: `\033[11~`
  - F2: `\033[12~`
  - F3: `\033[13~`
  - F4: `\033[14~`
  - F5: `\033[15~`
  - F6: `\033[17~`
  - F7: `\033[18~`
  - F8: `\033[19~`
  - F9: `\033[20~`
  - F10: `\033[21~`
  - F11: `\033[23~`
  - F12: `\033[24~`

### Updated Code
The following C++ code extends the previous SDL2 terminal emulator to support the specified special keys by updating the key event handler. All other functionality (ANSI escape sequences, double-buffered rendering, dynamic resizing, etc.) remains unchanged.

### Key Changes
1. **Special Key Handling**:
   - Updated the `SDL_KEYDOWN` event handler in the main loop to detect special keys using `SDLK_` constants.
   - Added a `switch` statement to map keys to their ANSI escape sequences:
     - Arrow keys: `SDLK_UP` → `\033[A`, `SDLK_DOWN` → `\033[B`, etc.
     - Home/End: `SDLK_HOME` → `\033[H`, `SDLK_END` → `\033[F`.
     - Insert/Delete: `SDLK_INSERT` → `\033[2~`, `SDLK_DELETE` → `\033[3~`.
     - Page Up/Down: `SDLK_PAGEUP` → `\033[5~`, `SDLK_PAGEDOWN` → `\033[6~`.
     - F1–F12: `SDLK_F1` → `\033[11~`, `SDLK_F2` → `\033[12~`, ..., `SDLK_F12` → `\033[24~`.
   - Preserved existing handling for printable characters, Enter, Backspace, Ctrl+C, and Ctrl+D.
   - Used `write(master_fd, input.c_str(), input.size())` to send sequences to the slave pseudo-terminal.

2. **No Other Changes**:
   - The rest of the code (ANSI escape sequence parsing, double-buffered rendering, antialiased text, dynamic resizing, cursor management, job control) remains identical.
   - The key handling update only affects the input processing in the SDL event loop.

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
- **Special Key Support**:
  - **Arrow Keys**: Navigate command history in `bash` (Up/Down) or move the cursor in `vim` (Left/Right).
  - **Home/End**: Move to the start/end of the line in `bash` or `vim`.
  - **Insert**: Toggle insert mode in `vim` (if applicable).
  - **Delete**: Delete the character under the cursor in `vim` or `bash`.
  - **Page Up/Down**: Scroll in `less` or `vim` (e.g., `less file.txt`).
  - **F1–F12**: Trigger application-specific actions (e.g., F1 for help in `less`, F10 to quit in `nano`).
- **Rendering**:
  - Text remains antialiased with double buffering, ensuring flicker-free display with ANSI colors and attributes.
- **SDL Window**:
  - Opens a window (initially 80x24 characters) with a `/bin/sh` prompt.
  - Resizing dynamically adjusts the slave terminal size.
- **Dynamic Resizing**:
  - Resizing updates `term_cols` and `term_rows`, and `stty size` reflects the new dimensions.
- **Key Input**:
  - Printable characters, Enter, Backspace, Ctrl+C, Ctrl+D, and now special keys (arrow, Home, End, etc.) work correctly.
- **Slave Output**:
  - Shell output with ANSI sequences (e.g., `ls --color`, `vim`) displays with colors and cursor positioning.
- **Job Control**:
  - Job control (`sleep 100 &`, `fg`, `bg`) and signal handling (Ctrl+C) work as before.
  - No "can't access tty" warnings.

### Testing
1. **Arrow Keys**:
   - In `bash`, press Up/Down to navigate command history, Left/Right to move the cursor.
   - In `vim`, use arrow keys to move the cursor in normal mode.
2. **Home/End**:
   - In `bash`, press Home to move to the start of the command, End to the end.
   - In `vim`, verify Home moves to the line start, End to the line end.
3. **Insert/Delete**:
   - In `vim`, press Insert to toggle insert mode, Delete to remove the character under the cursor.
   - In `bash`, press Delete to delete the character at the cursor.
4. **Page Up/Down**:
   - In `less file.txt`, press Page Up/Down to scroll.
   - In `vim`, verify Page Up/Down scrolls the view.
5. **F1–F12**:
   - In `less`, press F1 to view help.
   - In `nano`, press F10 to exit.
   - Test F1–F12 in `vim` for custom mappings (if set).
6. **ANSI Colors**:
   - Run `ls --color` to verify colored output.
   - Test `echo -e "\033[31mRed\033[0m"` for color changes.
7. **Rendering**:
   - Run `vim` or `top` to verify colors, cursor movement, and flicker-free rendering.
8. **Dynamic Resizing**:
   - Resize the window and run `stty size` to confirm dimensions.
   - Verify `vim` adjusts with correct colors and key behavior.
9. **Job Control**:
   - Run `sleep 100 &` and use `fg` or `bg`.
   - Press Ctrl+Z to suspend and resume.

### Notes
- **Font Path**: Update `TTF_OpenFont` if the font fails to load (e.g., `LiberationMono.ttf` on Linux or another `.ttc` on macOS).
- **Key Sequences**: The escape sequences are xterm/vt100-compatible, suitable for `bash`, `zsh`, `vim`, etc. Some applications may expect different sequences (e.g., `tmux`); I can adjust for specific cases if needed.
- **Rendering Limitation**: Each line uses a single color/attribute (first character’s). Per-character colors require a more complex renderer, which I can implement if desired.
- **Performance**: The double-buffered renderer with texture caching ensures smooth updates. Special key handling adds minimal overhead.
- **Unsupported Keys**: Other keys (e.g., Tab, Alt-modified keys) are not handled but can be added if required.

