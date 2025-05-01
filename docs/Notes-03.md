# Version 3

To handle `TIOCSWINSZ` in the terminal emulator, we need to ensure the slave pseudo-terminal's window size is properly set to match the parent terminal's size and updated when the parent terminal is resized. This allows the shell running in the slave to correctly handle commands that depend on terminal dimensions (e.g., `vim`, `less`, or `ls` with column formatting). Additionally, we need to handle the `SIGWINCH` signal to detect when the parent terminal's size changes and update the slave accordingly.

### Enhancements
- Use `ioctl(TIOCGWINSZ)` to get the parent terminal's window size and `ioctl(TIOCSWINSZ)` to set the slave's window size during initialization.
- Set up a `SIGWINCH` signal handler in the parent|The shell will now respect the terminal's size for proper formatting and behavior in programs like `vim` or `top`.
- Update the slave's window size whenever the parent terminal is resized.

Below is the updated C++ code, incorporating `TIOCSWINSZ` handling and `SIGWINCH` signal processing, building on the previous version:

### Key Changes
1. **Initial Window Size Setup**:
   - Added `ioctl(STDIN_FILENO, TIOCGWINSZ, &ws)` to get the parent terminal's window size before forking.
   - In the child process, used `ioctl(slave_fd, TIOCSWINSZ, &ws)` to set the slave's window size to match the parent’s.
2. **SIGWINCH Handler**:
   - Added a `handle_sigwinch` function to handle the `SIGWINCH` signal, which is triggered when the parent terminal is resized.
   - The handler uses `ioctl(TIOCGWINSZ)` to get the updated window size and `ioctl(TIOCSWINSZ)` to apply it to the slave (via `master_fd`).
   - Made `master_fd` a global variable so the signal handler can access it.
3. **Signal Registration**:
   - Registered the `SIGWINCH` handler with `signal(SIGWINCH, handle_sigwinch)` in the parent process.

### How to Compile and Run
1. Save the code as `terminal_emulator.cpp`.
2. Compile with g++:
   ```bash
   g++ -o terminal_emulator terminal_emulator.cpp
   ```
3. Run the program:
   ```bash
   ./terminal_emulator
   ```

### Expected Behavior
- The shell (`/bin/sh`) should now respect the terminal's window size, allowing proper formatting in commands like `ls`, `vim`, or `top`.
- Resizing the parent terminal (e.g., dragging the window or maximizing it) triggers `SIGWINCH`, and the slave's window size is updated immediately.
- Job control and signal handling (e.g., Ctrl+C) continue to work as in the previous version.
- No "can't access tty; job control turned off" warning should appear.

### Testing the Window Size Handling
- Run the emulator and try resizing the terminal window.
- Run commands like `stty size` in the shell to verify the terminal dimensions.
- Open `vim` or `less` to confirm that they adjust to the terminal size correctly.
- Run `resize` or `tput cols`/`tput lines` to check the reported terminal size.

### Notes
- The emulator now handles window resizing dynamically, but it still lacks advanced features like full ANSI escape sequence processing or support for complex terminal emulations (e.g., xterm). If you need these, let me know!
- If the terminal size cannot be retrieved or set, the program prints an error but continues running. In a production environment, you might want to handle such errors more gracefully.
- The code assumes a POSIX-compliant Linux environment with `/bin/sh` available.

