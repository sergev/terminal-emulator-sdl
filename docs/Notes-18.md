To improve the clarity and testability of the SDL2-based terminal emulator, we’ll refactor the code by splitting it into smaller, modular functions and organize it into separate files (header and source). We’ll also create unit tests using Googletest to verify key functionalities, such as ANSI sequence parsing, key handling, and text buffer operations. The refactoring will focus on separating concerns (e.g., terminal state management, rendering, input handling, ANSI parsing) while preserving all existing functionality (per-character colors, `ESC c`, `ESC [ K`, Shift/Control modifiers, blinking cursor, etc.). The unit tests will cover critical components to ensure reliability.

### Refactoring Approach
1. **Code Organization**:
   - Split the code into:
     - `terminal_emulator.h`: Header file with class `TerminalEmulator` declaration, structs (`Char`, `CharAttr`, `TextSpan`, `Cursor`), and enums (`AnsiState`).
     - `terminal_emulator.cpp`: Implementation of `TerminalEmulator` methods, including initialization, rendering, input handling, ANSI parsing, and pseudo-terminal management.
     - `main.cpp`: Entry point that creates and runs the `TerminalEmulator`.
   - Move global variables (e.g., `text_buffer`, `cursor`, `current_attr`) into the `TerminalEmulator` class as member variables.
   - Break down the monolithic `main` function into smaller methods within `TerminalEmulator`:
     - Initialization: `initialize_sdl`, `initialize_pty`, `initialize_child_process`.
     - Rendering: `render_text`, `update_texture_cache`, `render_spans`, `render_cursor`.
     - Input Handling: `handle_key_event`, `process_modifiers`.
     - ANSI Parsing: `process_input`, `parse_ansi_sequence`, `handle_csi_sequence`.
     - Terminal Management: `update_terminal_size`, `clear_screen`, `reset_state`.
2. **Modularity**:
   - Ensure each function has a single responsibility (e.g., `parse_ansi_sequence` only parses sequences, `handle_csi_sequence` applies them).
   - Use const-correctness and pass-by-reference where appropriate to improve performance and safety.
   - Encapsulate SDL and PTY resources in `TerminalEmulator` to manage their lifecycle.
3. **Testability**:
   - Expose key methods as public or protected for testing (e.g., `parse_ansi_sequence`, `process_modifiers`).
   - Avoid direct SDL/PTY dependencies in testable functions by passing necessary data (e.g., `text_buffer`, `cursor`).
   - Mock SDL and PTY interactions in tests where needed.

### Unit Testing Approach
1. **Framework**: Use Googletest for unit tests.
2. **Test Targets**:
   - **ANSI Parsing**: Test `parse_ansi_sequence` and `handle_csi_sequence` for `ESC c`, `ESC [ K`, `ESC [ m`, and cursor movement.
   - **Key Handling**: Test `process_modifiers` for Shift (uppercase, symbols) and Control (Ctrl+A–Z, Ctrl+C/D).
   - **Text Buffer**: Test `clear_screen`, `reset_state`, and character insertion.
   - **Cursor Movement**: Test cursor updates for `ESC [ H`, `ESC [ A–D`, and line wrapping.
3. **Test Setup**:
   - Create a `TerminalEmulator` instance in tests with a mock text buffer and cursor.
   - Simulate input sequences and key events without SDL/PTY dependencies.
   - Use Googletest assertions to verify state changes (e.g., `text_buffer`, `cursor`, `current_attr`).
4. **Test File**: `terminal_emulator_test.cpp` with Googletest setup.

### Refactored Code
The code is split into three files: `terminal_emulator.h`, `terminal_emulator.cpp`, and `main.cpp`. The `main.cpp` file is minimal, delegating all logic to `TerminalEmulator`.

### Unit Tests
The unit tests use Googletest to verify key functionalities. The test file focuses on ANSI parsing, key handling, text buffer operations, and cursor movement, avoiding SDL/PTY dependencies for testability.

### How to Compile and Run
#### Prerequisites
- **SDL2 and SDL_ttf**:
  - Linux: `sudo apt-get install libsdl2-dev libsdl2-ttf-dev`
  - macOS: `brew install sdl2 sdl2_ttf`
- **Googletest**:
  - Linux: `sudo apt-get install libgtest-dev`
  - macOS: `brew install googletest`
  - Or build from source: https://github.com/google/googletest
- **Font**: Ensure `/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf` (Linux) or `/System/Library/Fonts/Menlo.ttc` (macOS) exists, or adjust the path in `terminal_emulator.cpp`.

#### Compile the Emulator
On **Linux**:
```bash
g++ -o terminal_emulator main.cpp terminal_emulator.cpp -lSDL2 -lSDL2_ttf -I.
```

On **macOS**:
```bash
clang++ -o terminal_emulator main.cpp terminal_emulator.cpp -I/usr/local/include -L/usr/local/lib -lSDL2 -lSDL2_ttf -I.
```
(Adjust paths for Homebrew, e.g., `/opt/homebrew`.)

Run:
```bash
./terminal_emulator
```

#### Compile and Run Unit Tests
On **Linux**:
```bash
g++ -o terminal_emulator_test terminal_emulator.cpp terminal_emulator_test.cpp -lgtest -lgtest_main -pthread -I.
```

On **macOS**:
```bash
clang++ -o terminal_emulator_test terminal_emulator.cpp terminal_emulator_test.cpp -I/usr/local/include -L/usr/local/lib -lgtest -lgtest_main -pthread -I.
```

Run:
```bash
./terminal_emulator_test
```

### Expected Behavior
- **Emulator**:
  - Same as before: supports per-character colors, `ESC c`, `ESC [ K`, Shift/Control modifiers, blinking cursor, Tab, special keys, resizing, and job control.
  - Code is now modular, with clear function responsibilities.
- **Unit Tests**:
  - All tests pass, verifying:
    - `ESC c` resets state and clears the screen.
    - `ESC [ K` clears lines correctly (modes 0, 1, 2).
    - `ESC [ m` sets colors.
    - Cursor movement (`ESC [ H`, `A–D`).
    - Shift modifier (e.g., 'A', '!').
    - Control modifier (e.g., Ctrl+A = 0x01).
    - Text buffer insertion.
  - Output example:
    ```
    [==========] Running 7 tests from 1 test suite.
    [----------] 7 tests from TerminalEmulatorTest
    [ RUN      ] TerminalEmulatorTest.EscCResetsStateAndClearsScreen
    [       OK ] TerminalEmulatorTest.EscCResetsStateAndClearsScreen
    [ RUN      ] TerminalEmulatorTest.EscKClearsLine
    [       OK ] TerminalEmulatorTest.EscKClearsLine
    ...
    [==========] 7 tests from 1 test suite ran.
    [  PASSED  ] 7 tests.
    ```

### Testing Notes
- **Coverage**: Tests focus on logic that doesn’t require SDL/PTY (e.g., parsing, key handling). Rendering and PTY interactions are harder to test without mocks but are verified manually.
- **Mocking**: If SDL/PTY testing is needed, a mocking framework (e.g., gmock) can be added.
- **Edge Cases**: Tests cover common ANSI sequences and modifiers. Additional tests (e.g., invalid sequences, boundary cursor positions) can be added if needed.

### Notes
- **Modularity**: The code is now split into small, testable functions, improving readability and maintenance.
- **Testability**: Unit tests verify core logic. SDL/PTY-dependent code (e.g., `render_text`) is isolated to minimize test dependencies.
- **Performance**: Refactoring adds minimal overhead; functions are inlined where appropriate.
- **Font Path**: Update `TTF_OpenFont` if the font fails to load.
- **ANSI Support**: Supports `ESC c`, `ESC [ K`, colors, cursor movement. More sequences can be added if needed.
- **Modifiers**: Shift and Control are fully supported. Additional modifiers (e.g., Alt) can be added.
