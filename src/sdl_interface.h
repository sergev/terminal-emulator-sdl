//
// Terminal emulator: interface to Unix and graphics library.
//
// Copyright (c) 2025 Serge Vakulenko
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#ifndef SDL_INTERFACE_H
#define SDL_INTERFACE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string>
#include <vector>

#include "terminal_logic.h"

#ifdef __linux__
#include <pty.h>
#else
#include <util.h>
#endif

// Structure for a span of characters with the same attributes
struct TextSpan {
    std::wstring text; // Use wstring for Unicode
    CharAttr attr;
    int start_col;
    SDL_Texture *texture = nullptr;
};

class SdlInterface {
public:
    SdlInterface(int cols = 80, int rows = 24);
    ~SdlInterface();
    bool initialize();
    void run();

private:
    // Terminal state
    int term_cols;
    int term_rows;
    std::vector<std::vector<TextSpan>> texture_cache;
    std::vector<bool> dirty_lines;
    int font_size{ 16 }; // Current font size in points
    std::string font_path;

    // SDL resources
    SDL_Window *window{};
    SDL_Renderer *renderer{};
    TTF_Font *font{};
    int char_width{};
    int char_height{};
    bool cursor_visible{ true };
    Uint32 last_cursor_toggle{};
    static const Uint32 cursor_blink_interval = 500;

    // PTY and child process
    int master_fd{ -1 };
    pid_t child_pid{};

    // Terminal logic
    TerminalLogic terminal_logic;

    // Initialization methods
    bool initialize_sdl();
    bool initialize_pty(struct termios &slave_termios, char *&slave_name);
    bool initialize_child_process(const char *slave_name, const struct termios &slave_termios);

    // Rendering methods
    void render_text();
    void update_texture_cache();
    void render_spans();
    void render_cursor();

    // Input handling methods
    void handle_events();
    void handle_key_event(const SDL_KeyboardEvent &key);
    void change_font_size(int delta);
    static KeyInput keysym_to_key_input(const SDL_Keysym &keysym);

    // PTY input handling
    void process_pty_input();

    // Signal handlers
    static void forward_signal(int sig);
    static void handle_sigwinch(int sig);
};

#endif // SDL_INTERFACE_H
