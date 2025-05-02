//
// ANSI logic of the terminal emulator.
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
#ifndef TERMINAL_LOGIC_H
#define TERMINAL_LOGIC_H

#include <gtest/gtest_prod.h>

#include <cstdint>
#include <cwchar>
#include <map>
#include <string>
#include <vector>

// Device-independent keycodes
enum class KeyCode {
    // clang-format off
    UNKNOWN,
    ENTER,
    BACKSPACE,
    TAB,
    ESCAPE,
    UP, DOWN, RIGHT, LEFT,
    HOME, END,
    INSERT, DELETE,
    PAGEUP, PAGEDOWN,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    CHARACTER, // For printable characters
    // clang-format off
};

// Structure for key input
struct KeyInput {
    KeyCode code{ KeyCode::UNKNOWN };
    wchar_t character{};
    bool mod_shift{};
    bool mod_ctrl{};

    KeyInput() = default;
    KeyInput(unsigned c, bool shift, bool ctrl) :
        code(KeyCode::CHARACTER), character(c), mod_shift(shift), mod_ctrl(ctrl) {}
};

// Structure for character attributes
struct CharAttr {
    uint8_t fg_r = 255, fg_g = 255, fg_b = 255, fg_a = 255; // Foreground color (default white)
    uint8_t bg_r = 0, bg_g = 0, bg_b = 0, bg_a = 255;       // Background color (default black)

    bool operator==(const CharAttr &other) const
    {
        return fg_r == other.fg_r && fg_g == other.fg_g && fg_b == other.fg_b &&
               fg_a == other.fg_a && bg_r == other.bg_r && bg_g == other.bg_g &&
               bg_b == other.bg_b && bg_a == other.bg_a;
    }
};

// Structure for a single character with attributes
struct Char {
    wchar_t ch = L' '; // Use wchar_t for Unicode
    CharAttr attr;
};

// Structure for a span of characters with the same attributes
struct TextSpan {
    std::wstring text; // Use wstring for Unicode
    CharAttr attr;
    int start_col;
    void *texture = nullptr; // Opaque pointer for SdlInterface
};

// Cursor position
struct Cursor {
    int row = 0;
    int col = 0;
};

// ANSI parsing states
enum class AnsiState { NORMAL, ESCAPE, CSI };

class TerminalLogic {
public:
    TerminalLogic(int cols = 80, int rows = 24);
    void resize(int new_cols, int new_rows);
    std::vector<int> process_input(const char *buffer, size_t length);
    std::string process_key(const KeyInput &key);
    const std::vector<std::vector<Char>> &get_text_buffer() const;
    const Cursor &get_cursor() const;

private:
    // Declare test cases as friends
    FRIEND_TEST(TerminalLogicTest, EscCResetsStateAndClearsScreen);
    FRIEND_TEST(TerminalLogicTest, EscKClearsLine);
    FRIEND_TEST(TerminalLogicTest, EscMSetsColors);
    FRIEND_TEST(TerminalLogicTest, CursorMovement);
    FRIEND_TEST(TerminalLogicTest, ShiftModifier);
    FRIEND_TEST(TerminalLogicTest, ControlModifier);
    FRIEND_TEST(TerminalLogicTest, TextBufferInsertion);
    FRIEND_TEST(TerminalLogicTest, ScrollUp);
    FRIEND_TEST(TerminalLogicTest, ClearScreenEsc0J);
    FRIEND_TEST(TerminalLogicTest, ClearScreenEsc1J);
    FRIEND_TEST(TerminalLogicTest, ClearScreenEsc2J);

    // Terminal state
    int term_cols;
    int term_rows;
    std::vector<std::vector<Char>> text_buffer;
    Cursor cursor;
    CharAttr current_attr;
    AnsiState state;
    std::string ansi_seq;

    // ANSI colors
    static const CharAttr ansi_colors[];

    // ANSI parsing methods
    void parse_ansi_sequence(const std::string &seq, char final_char);
    void handle_csi_sequence(const std::string &seq, char final_char,
                             const std::vector<int> &params);

    // Terminal management methods
    void clear_screen();
    void reset_state();
    void scroll_up();
};

#endif // TERMINAL_LOGIC_H
