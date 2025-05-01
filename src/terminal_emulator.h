//
// Simple ANSI terminal emulator.
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
#ifndef TERMINAL_EMULATOR_H
#define TERMINAL_EMULATOR_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <map>
#include <string>
#include <vector>

// Include gtest_prod.h for FRIEND_TEST macro
#include <gtest/gtest_prod.h>

#ifdef __linux__
#include <pty.h>
#else
#include <util.h>
#endif

// Structure for character attributes
struct CharAttr {
    SDL_Color fg = { 255, 255, 255, 255 }; // Foreground color (default white)
    SDL_Color bg = { 0, 0, 0, 255 };       // Background color (default black)

    bool operator==(const CharAttr &other) const
    {
        return fg.r == other.fg.r && fg.g == other.fg.g && fg.b == other.fg.b &&
               fg.a == other.fg.a && bg.r == other.bg.r && bg.g == other.bg.g &&
               bg.b == other.bg.b && bg.a == other.bg.a;
    }
};

// Structure for a single character with attributes
struct Char {
    char ch = ' ';
    CharAttr attr;
};

// Structure for a span of characters with the same attributes
struct TextSpan {
    std::string text;
    CharAttr attr;
    int start_col;
    SDL_Texture *texture = nullptr;
};

// Cursor position
struct Cursor {
    int row = 0;
    int col = 0;
};

// ANSI parsing states
enum class AnsiState { NORMAL, ESCAPE, CSI };

class TerminalEmulator {
public:
    TerminalEmulator(int cols = 80, int rows = 24);
    ~TerminalEmulator();
    bool initialize();
    void run();

private:
    // Allow tests to access private members
    friend class TerminalEmulatorTest;

    // Declare each test case as a friend using FRIEND_TEST
    FRIEND_TEST(TerminalEmulatorTest, EscCResetsStateAndClearsScreen);
    FRIEND_TEST(TerminalEmulatorTest, EscKClearsLine);
    FRIEND_TEST(TerminalEmulatorTest, EscMSetsColors);
    FRIEND_TEST(TerminalEmulatorTest, CursorMovement);
    FRIEND_TEST(TerminalEmulatorTest, ShiftModifier);
    FRIEND_TEST(TerminalEmulatorTest, ControlModifier);
    FRIEND_TEST(TerminalEmulatorTest, TextBufferInsertion);

    // Terminal state
    int term_cols;
    int term_rows;
    std::vector<std::vector<Char>> text_buffer;
    std::vector<std::vector<TextSpan>> texture_cache;
    std::vector<bool> dirty_lines;
    Cursor cursor;
    CharAttr current_attr;
    AnsiState state{ AnsiState::NORMAL };
    std::string ansi_seq;
    int font_size{ 16 };   // Current font size in points
    std::string font_path; // Font file path

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

    // ANSI colors
    static const SDL_Color ansi_colors[];

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
    void process_modifiers(const SDL_KeyboardEvent &key, std::string &input);
    void change_font_size(int delta);

    // ANSI parsing methods
    void process_input();
    void parse_ansi_sequence(const std::string &seq, char final_char);
    void handle_csi_sequence(const std::string &seq, char final_char,
                             const std::vector<int> &params);

    // Terminal management methods
    void update_terminal_size();
    void clear_screen();
    void reset_state();
    void scroll_up();

    // Signal handlers
    static void forward_signal(int sig);
    static void handle_sigwinch(int sig);
};

#endif // TERMINAL_EMULATOR_H
