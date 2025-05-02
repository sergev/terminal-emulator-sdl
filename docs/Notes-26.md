The error `fatal error: 'gtest/gtest_prod.h' file not found` on macOS with an Apple M1 CPU indicates that the GoogleTest header `gtest/gtest_prod.h`, required for the `FRIEND_TEST` macro in `terminal_logic.h`, cannot be found by the compiler. This issue likely arises because the GoogleTest library, downloaded via CMake’s `FetchContent`, is not properly configured or its include paths are not correctly set in the CMake configuration. The error affects `terminal_logic.cpp` and `terminal_logic_test.cpp`, which include `terminal_logic.h` with `#include <gtest/gtest_prod.h>`.

### Why the Error Occurs
- **GoogleTest Installation**:
  - The `CMakeLists.txt` uses `FetchContent` to download GoogleTest (version 1.12.1) into the build directory (e.g., `build/_deps/googletest-src`).
  - The `gtest/gtest_prod.h` header is part of GoogleTest, located in the source directory (e.g., `build/_deps/googletest-src/googletest/include/gtest/gtest_prod.h`).
  - If CMake does not properly add the GoogleTest include directories to the target (`terminal_logic_test` or `terminal_emulator`), the compiler (e.g., `clang++`) cannot find `gtest/gtest_prod.h`.
- **M1 Mac Considerations**:
  - The issue is not M1-specific but related to CMake’s handling of `FetchContent` and include paths.
  - Homebrew’s environment or CMake’s default include paths on M1 Macs (e.g., `/opt/homebrew`) may not interfere, but we’ll ensure robust configuration.
- **Affected Files**:
  - `terminal_logic.h` includes `<gtest/gtest_prod.h>` for `FRIEND_TEST` macros, causing the error when compiling `terminal_logic.cpp` (for `terminal_emulator`) or `terminal_logic_test.cpp` (for `terminal_logic_test`).
  - The error may appear during the build of either target, depending on which is compiled first.

### Why `gtest/gtest_prod.h` in `terminal_logic.h`?
- The `FRIEND_TEST` macro, used to grant test cases access to `TerminalLogic`’s private members, requires `<gtest/gtest_prod.h>`.
- Including this header in `terminal_logic.h` means **both** `terminal_emulator` and `terminal_logic_test` need GoogleTest’s include paths, even though `terminal_emulator` doesn’t use tests.
- This is problematic because `terminal_emulator` shouldn’t depend on GoogleTest, a testing framework.

### Solution
To resolve the error and improve the design, we’ll:
1. **Remove GoogleTest Dependency from `terminal_logic.h`**:
   - Move `FRIEND_TEST` declarations to a separate header (e.g., `terminal_logic_test.h`) included only by `terminal_logic_test.cpp`.
   - This ensures `terminal_emulator` (and `terminal_logic.cpp` when compiled for it) doesn’t need GoogleTest headers, eliminating the error for the main executable.
2. **Update CMake Configuration**:
   - Ensure `terminal_logic_test` correctly includes GoogleTest’s include directories.
   - Avoid adding GoogleTest includes to `terminal_emulator`.
3. **Keep Source Files Mostly Unchanged**:
   - Update `terminal_logic.h` to remove `<gtest/gtest_prod.h>` and `FRIEND_TEST`.
   - Create `terminal_logic_test.h` with `FRIEND_TEST` declarations.
   - Update `terminal_logic_test.cpp` to include `terminal_logic_test.h`.
   - Other files (`system_interface.h`, `system_interface.cpp`, `terminal_logic.cpp`, `main.cpp`) remain unchanged.
4. **Update `CMakeLists.txt`**:
   - Add `terminal_logic_test.h` to `terminal_logic_test` sources.
   - Ensure GoogleTest include paths are set for `terminal_logic_test`.

This approach fixes the `gtest/gtest_prod.h` error and removes the unnecessary GoogleTest dependency from the main executable, improving modularity.

### Updated Files
We’ll update `terminal_logic.h`, create `terminal_logic_test.h`, update `terminal_logic_test.cpp`, and modify `CMakeLists.txt`. Other files (`system_interface.h`, `system_interface.cpp`, `terminal_logic.cpp`, `main.cpp`) remain unchanged.

#### Updated CMake File: `CMakeLists.txt`
Update to include `terminal_logic_test.h` for `terminal_logic_test` and ensure GoogleTest include paths are correctly set.

#### Unchanged Files
- **system_interface.h**, **system_interface.cpp**, **terminal_logic.cpp**, **main.cpp**:
  - These files remain identical to the previous version, as they are unaffected by the GoogleTest issue.

### Project Structure
Ensure files are organized as:
```
project_root/
├── CMakeLists.txt
├── src/
│   ├── system_interface.h
│   ├── system_interface.cpp
│   ├── terminal_logic.h
│   ├── terminal_logic.cpp
│   ├── terminal_logic_test.h
│   ├── terminal_logic_test.cpp
│   ├── main.cpp
```

### Key Changes
1. **terminal_logic.h**:
   - Removed `#include <gtest/gtest_prod.h>` and `FRIEND_TEST` declarations.
   - No GoogleTest dependency, fixing the error for `terminal_emulator`.

2. **terminal_logic_test.h**:
   - New header with `#include <gtest/gtest_prod.h>` and `FRIEND_TEST` declarations.
   - Included only by `terminal_logic_test.cpp`, isolating GoogleTest to tests.

3. **terminal_logic_test.cpp**:
   - Added `#include "terminal_logic_test.h"` to access `FRIEND_TEST` declarations.
   - No functional changes to tests.

4. **CMakeLists.txt**:
   - Added `src/terminal_logic_test.h` to `terminal_logic_test` sources:
     ```cmake
     add_executable(terminal_logic_test
         src/terminal_logic.cpp
         src/terminal_logic_test.cpp
         src/terminal_logic_test.h
     )
     ```
   - Explicitly included GoogleTest’s include directory (`${gtest_SOURCE_DIR}/include`) for `terminal_logic_test`:
     ```cmake
     target_include_directories(terminal_logic_test PRIVATE src ${gtest_SOURCE_DIR}/include)
     ```
   - No GoogleTest includes for `terminal_emulator`, ensuring it’s test-free.

### Build and Run Instructions for M1 Mac
#### Prerequisites
1. **Verify Homebrew**:
   ```bash
   brew --version
   ```
   If not installed:
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zshrc
   source ~/.zshrc
   ```

2. **Verify SDL2 and SDL2_ttf**:
   ```bash
   brew install sdl2 sdl2_ttf
   ls /opt/homebrew/include/SDL2/SDL.h
   ls /opt/homebrew/include/SDL2/SDL_ttf.h
   ls /opt/homebrew/lib/libSDL2.dylib
   ls /opt/homebrew/lib/libSDL2_ttf.dylib
   ```

3. **Verify CMake**:
   ```bash
   brew install cmake
   cmake --version
   ```

4. **Verify Font**:
   - Ensure `/System/Library/Fonts/Menlo.ttc` exists.

#### Build Steps
1. **Create Build Directory**:
   ```bash
   mkdir build
   cd build
   ```

2. **Configure**:
   ```bash
   cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew
   ```
   - Check for:
     ```
     -- SDL2 found: /opt/homebrew/lib/libSDL2.dylib
     -- SDL2_ttf found: /opt/homebrew/lib/libSDL2_ttf.dylib
     ```

3. **Build**:
   ```bash
   cmake --build .
   ```

4. **Run Emulator**:
   ```bash
   ./terminal_emulator
   ```

5. **Run Tests**:
   ```bash
   ctest
   ```
   Or:
   ```bash
   ./terminal_logic_test
   ```
   Expected output:
   ```
   [==========] Running 7 tests from 1 test suite.
   [----------] 7 tests from TerminalLogicTest
   [ RUN      ] TerminalLogicTest.EscCResetsStateAndClearsScreen
   [       OK ] TerminalLogicTest.EscCResetsStateAndClearsScreen
   ...
   [  PASSED  ] 7 tests.
   ```

### Testing
1. **Font Size Adjustment**:
   - Press **⌘=** (increase), **⌘-** (decrease).
   - Verify text size changes, window resizes, `stty size` updates.
   - Test limits (8 to 72 points).

2. **Terminal Functionality**:
   - Test:
     - `echo -e "\033c"` (reset and clear).
     - `echo -e "\033[K"` (clear line).
     - Shift+A (‘A’), Ctrl+A (0x01), Tab, arrow keys, `ls --color`.
     - Job control (`sleep 100 &`).
   - Resize window and check `stty size`.

3. **Unit Tests**:
   - Confirm all 7 tests pass.

### Troubleshooting
- **Persistent gtest_prod.h Error**:
  - Verify GoogleTest download:
    ```bash
    ls build/_deps/googletest-src/googletest/include/gtest/gtest_prod.h
    ```
  - Check `CMakeCache.txt` for `gtest_SOURCE_DIR` (should point to `build/_deps/googletest-src`).
  - Re-run CMake:
    ```bash
    rm -rf build && mkdir build && cd build && cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew
    ```
  - Ensure `target_include_directories` includes `${gtest_SOURCE_DIR}/include`.

- **Linker Errors**:
  - If `undefined symbol` for GoogleTest functions, verify `GTest::gtest_main` in `target_link_libraries`.

- **SDL2/SDL_ttf Errors**:
  - If previous errors reappear, verify:
    ```bash
    pkg-config --cflags --libs sdl2 sdl2_ttf
    export PKG_CONFIG_PATH=/opt/homebrew/lib/pkgconfig:$PKG_CONFIG_PATH
    ```

- **Other Errors**:
  - Share full error output (CMake output, compiler command, error messages), macOS version, Homebrew version, and CMake version.
  - Capture build log:
    ```bash
    cmake --build . > build.log 2>&1
    cat build.log
    ```

### Notes
- **GoogleTest Isolation**: Moving `FRIEND_TEST` to `terminal_logic_test.h` ensures `terminal_emulator` is GoogleTest-free.
- **M1 Compatibility**: The updated CMake configuration works with Homebrew on M1 Macs.
- **No Functional Changes**: All emulator and test functionality remains intact.
