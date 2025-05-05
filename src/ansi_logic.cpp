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
#include "ansi_logic.h"

#include <unicode/uchar.h>

#include <algorithm>
#include <cctype>
#include <iostream>

const RgbColor AnsiLogic::normal_colors[8] = {
    { 0, 0, 0 },       // Black
    { 192, 0, 0 },     // Red
    { 0, 192, 0 },     // Green
    { 192, 85, 0 },    // Yellow (Brown)
    { 0, 0, 192 },     // Blue
    { 192, 0, 192 },   // Magenta
    { 0, 192, 192 },   // Cyan
    { 192, 192, 192 }, // White (Light Gray)
};

const RgbColor AnsiLogic::bright_colors[8] = {
    { 85, 85, 85 },    // Bright Black (Gray)
    { 255, 0, 0 },     // Bright Red
    { 0, 255, 0 },     // Bright Green
    { 255, 255, 0 },   // Bright Yellow
    { 0, 0, 255 },     // Bright Blue
    { 255, 0, 255 },   // Bright Magenta
    { 0, 255, 255 },   // Bright Cyan
    { 255, 255, 255 }, // Bright White
};

AnsiLogic::AnsiLogic(int cols, int rows)
    : term_cols(cols), term_rows(rows), state(AnsiState::NORMAL)
{
    text_buffer.resize(term_rows, std::vector<Char>(term_cols, { L' ', current_attr }));
}

void AnsiLogic::resize(int new_cols, int new_rows)
{
    term_cols = new_cols;
    term_rows = new_rows;
    text_buffer.resize(term_rows, std::vector<Char>(term_cols, { L' ', current_attr }));
    for (auto &line : text_buffer) {
        line.resize(term_cols, { L' ', current_attr });
    }
    cursor.row = std::min(cursor.row, term_rows - 1);
    cursor.col = std::min(cursor.col, term_cols - 1);
}

std::vector<int> AnsiLogic::process_input(const char *buffer, size_t length)
{
    std::vector<int> dirty_rows;
    size_t i = 0;
    while (i < length) {
        char c = buffer[i];
        switch (state) {
        case AnsiState::NORMAL:
            switch (c) {
            case '\033':
                state = AnsiState::ESCAPE;
                ansi_seq.clear();
                // std::cerr << "Received ESC, transitioning to ESCAPE state" << std::endl;
                ++i;
                break;
            case '\n':
                cursor.row++;
                cursor.col = 0;
                if (cursor.row >= term_rows) {
                    scroll_up();
                    for (int r = 0; r < term_rows; ++r) {
                        dirty_rows.push_back(r);
                    }
                } else {
                    dirty_rows.push_back(cursor.row);
                }
                ++i;
                break;
            case '\r':
                cursor.col = 0;
                if (i + 1 < length && buffer[i + 1] == '\n') {
                    ++i;
                    cursor.row++;
                    if (cursor.row >= term_rows) {
                        scroll_up();
                        for (int r = 0; r < term_rows; ++r) {
                            dirty_rows.push_back(r);
                        }
                    } else {
                        dirty_rows.push_back(cursor.row);
                    }
                } else {
                    dirty_rows.push_back(cursor.row);
                }
                ++i;
                break;
            case '\b':
                if (cursor.col > 0) {
                    cursor.col--;
                    text_buffer[cursor.row][cursor.col] = { L' ', current_attr };
                    dirty_rows.push_back(cursor.row);
                }
                ++i;
                break;
            case '\t':
                cursor.col = (cursor.col + 8) / 8 * 8;
                if (cursor.col >= term_cols) {
                    cursor.col = term_cols - 1;
                }
                ++i;
                break;
            case '\7':
                // TODO: make a sound.
                ++i;
                break;
            default:
                // Decode UTF-8 sequence
                wchar_t ch = 0;
                int bytes  = 0;
                if ((buffer[i] & 0x80) == 0) { // 1-byte (ASCII)
                    ch    = buffer[i];
                    bytes = 1;
                } else if ((buffer[i] & 0xE0) == 0xC0 && i + 1 < length) { // 2-byte
                    ch    = ((buffer[i] & 0x1F) << 6) | (buffer[i + 1] & 0x3F);
                    bytes = 2;
                } else if ((buffer[i] & 0xF0) == 0xE0 && i + 2 < length) { // 3-byte
                    ch = ((buffer[i] & 0x0F) << 12) | ((buffer[i + 1] & 0x3F) << 6) |
                         (buffer[i + 2] & 0x3F);
                    bytes = 3;
                } else if ((buffer[i] & 0xF8) == 0xF0 && i + 3 < length) { // 4-byte
                    ch = ((buffer[i] & 0x07) << 18) | ((buffer[i + 1] & 0x3F) << 12) |
                         ((buffer[i + 2] & 0x3F) << 6) | (buffer[i + 3] & 0x3F);
                    bytes = 4;
                } else {
                    // Invalid UTF-8, skip
                    ++i;
                    continue;
                }

                if (cursor.col < term_cols && cursor.row < term_rows) {
                    text_buffer[cursor.row][cursor.col] = { ch, current_attr };
                    cursor.col++;
                    dirty_rows.push_back(cursor.row);
                }
                if (cursor.col >= term_cols) {
                    cursor.col = 0;
                    cursor.row++;
                    if (cursor.row >= term_rows) {
                        scroll_up();
                        for (int r = 0; r < term_rows; ++r) {
                            dirty_rows.push_back(r);
                        }
                    }
                }
                i += bytes;
            }
            break;

        case AnsiState::ESCAPE:
            switch (c) {
            case '[':
                state = AnsiState::CSI;
                ansi_seq.clear();
                ansi_seq += c;
                // std::cerr << "Received [, transitioning to CSI state" << std::endl;
                break;
            case 'c':
                // std::cerr << "Received ESC c, processing reset" << std::endl;
                reset_state();
                for (int r = 0; r < term_rows; ++r) {
                    dirty_rows.push_back(r);
                }
                state = AnsiState::NORMAL;
                ansi_seq.clear();
                break;
            default:
                // std::cerr << "Unknown ESC sequence char: " << (int)c << ", resetting to NORMAL"
                //           << std::endl;
                state = AnsiState::NORMAL;
                ansi_seq.clear();
            }
            ++i;
            break;

        case AnsiState::CSI:
            ansi_seq += c;
            if (std::isalpha(c)) {
                // std::cerr << "Received CSI final char: " << c << std::endl;
                parse_ansi_sequence(ansi_seq, dirty_rows);
                state = AnsiState::NORMAL;
                ansi_seq.clear();
            }
            ++i;
            break;
        }
    }
    // Remove duplicates
    std::sort(dirty_rows.begin(), dirty_rows.end());
    dirty_rows.erase(std::unique(dirty_rows.begin(), dirty_rows.end()), dirty_rows.end());
    return dirty_rows;
}

static std::string wchar_to_utf8(wchar_t wc)
{
    std::string utf8;
    if (wc <= 0x7F) {
        utf8 += static_cast<char>(wc);
    } else if (wc <= 0x7FF) {
        utf8 += static_cast<char>(0xC0 | ((wc >> 6) & 0x1F));
        utf8 += static_cast<char>(0x80 | (wc & 0x3F));
    } else if (wc <= 0xFFFF) {
        utf8 += static_cast<char>(0xE0 | ((wc >> 12) & 0x0F));
        utf8 += static_cast<char>(0x80 | ((wc >> 6) & 0x3F));
        utf8 += static_cast<char>(0x80 | (wc & 0x3F));
    } else {
        utf8 += static_cast<char>(0xF0 | ((wc >> 18) & 0x07));
        utf8 += static_cast<char>(0x80 | ((wc >> 12) & 0x3F));
        utf8 += static_cast<char>(0x80 | ((wc >> 6) & 0x3F));
        utf8 += static_cast<char>(0x80 | (wc & 0x3F));
    }
    return utf8;
}

std::string AnsiLogic::process_key(const KeyInput &key)
{
    std::string input;
    // std::cerr << "Key processed: Keycode=" << key.code << ", Modifiers=" << modifiers <<
    // std::endl;

    // Map keycodes to terminal inputs
    switch (key.code) {
    case KeyCode::UNKNOWN:
    case KeyCode::CAPSLOCK:
    case KeyCode::LEFT_SHIFT:
    case KeyCode::RIGHT_SHIFT:
    case KeyCode::LEFT_CTRL:
    case KeyCode::RIGHT_CTRL:
    case KeyCode::LEFT_OPTION:
    case KeyCode::RIGHT_OPTION:
    case KeyCode::LEFT_COMMAND:
    case KeyCode::RIGHT_COMMAND:
        // No input.
        break;
    case KeyCode::ENTER:
        input = "\r";
        break;
    case KeyCode::BACKSPACE:
        input = "\b";
        break;
    case KeyCode::TAB:
        input = "\t";
        break;
    case KeyCode::ESCAPE:
        input = "\33";
        break;
    case KeyCode::UP:
        input = "\033[A";
        break;
    case KeyCode::DOWN:
        input = "\033[B";
        break;
    case KeyCode::RIGHT:
        input = "\033[C";
        break;
    case KeyCode::LEFT:
        input = "\033[D";
        break;
    case KeyCode::HOME:
        input = "\033[H";
        break;
    case KeyCode::END:
        input = "\033[F";
        break;
    case KeyCode::INSERT:
        input = "\033[2~";
        break;
    case KeyCode::DELETE:
        input = "\033[3~";
        break;
    case KeyCode::PAGEUP:
        input = "\033[5~";
        break;
    case KeyCode::PAGEDOWN:
        input = "\033[6~";
        break;
    case KeyCode::F1:
        input = "\033OP";
        break;
    case KeyCode::F2:
        input = "\033OQ";
        break;
    case KeyCode::F3:
        input = "\033OR";
        break;
    case KeyCode::F4:
        input = "\033OS";
        break;
    case KeyCode::F5:
        input = "\033[15~";
        break;
    case KeyCode::F6:
        input = "\033[17~";
        break;
    case KeyCode::F7:
        input = "\033[18~";
        break;
    case KeyCode::F8:
        input = "\033[19~";
        break;
    case KeyCode::F9:
        input = "\033[20~";
        break;
    case KeyCode::F10:
        input = "\033[21~";
        break;
    case KeyCode::F11:
        input = "\033[23~";
        break;
    case KeyCode::F12:
        input = "\033[24~";
        break;
    case KeyCode::CHARACTER:
        if (key.mod_ctrl) {
            //
            // Ctrl modifier is pressed.
            //
            input = std::string(1, key.character & 0x1f);
        } else if (key.mod_shift) {
            //
            // Shift modifier is pressed.
            //
            if (key.character <= 0x7f) {
                // ASCII symbol.
                char ch = key.character;
                if (key.character >= 'a' && key.character <= 'z') {
                    // Convert ASCII character to uppercase.
                    ch = std::toupper(ch);
                } else {
                    static const std::map<char, char> shift_map = {
                        { '1', '!' },  { '2', '@' }, { '3', '#' }, { '4', '$' }, { '5', '%' },
                        { '6', '^' },  { '7', '&' }, { '8', '*' }, { '9', '(' }, { '0', ')' },
                        { '-', '_' },  { '=', '+' }, { '[', '{' }, { ']', '}' }, { ';', ':' },
                        { '\'', '"' }, { ',', '<' }, { '.', '>' }, { '/', '?' }, { '`', '~' }
                    };
                    auto it = shift_map.find(ch);
                    if (it != shift_map.end()) {
                        ch = it->second;
                    }
                }
                input = std::string(1, ch);
            } else {
                // Convert Unicode character to uppercase.
                input = wchar_to_utf8(u_toupper(key.character));
            }
        } else if (key.character <= 0x7f) {
            //
            // ASCII symbol.
            //
            input = std::string(1, key.character);
        } else {
            //
            // Unicode symbol.
            //
            input = wchar_to_utf8(key.character);
        }
        break;
    }
    return input;
}

static int get_param(const std::vector<int> &params, int index, int default_value)
{
    if (params.size() > index && params[index] > default_value) {
        return params[index];
    }
    return default_value;
}

void AnsiLogic::parse_ansi_sequence(const std::string &seq, std::vector<int> &dirty_rows)
{
    if (seq.empty() || seq[0] != '[') {
        // std::cerr << "Invalid CSI sequence: " << seq << std::endl;
        return;
    }

    // std::cerr << "Processing CSI sequence: " << seq << std::endl;
    std::vector<int> params;
    std::string param_str;

    // Process all characters up to the final character
    for (size_t i = 1; i < seq.size(); ++i) {
        char c = seq[i];
        if (std::isdigit(c)) {
            param_str += c;
        } else if (c == ';' || std::isalpha(c)) {
            if (!param_str.empty()) {
                try {
                    params.push_back(std::stoi(param_str));
                } catch (const std::exception &e) {
                    // std::cerr << "Error parsing parameter '" << param_str << "': " << e.what()
                    //           << std::endl;
                    params.push_back(0);
                }
                param_str.clear();
            } else {
                params.push_back(0);
            }
            if (std::isalpha(c)) {
                break; // Final character reached
            }
        }
    }

    // Handle any remaining parameter
    if (!param_str.empty()) {
        try {
            params.push_back(std::stoi(param_str));
        } catch (const std::exception &e) {
            // std::cerr << "Error parsing final parameter '" << param_str << "': " << e.what()
            //           << std::endl;
            params.push_back(0);
        }
    }

    // Process final character
    switch (seq.back()) {
    case 'm': {
        const RgbColor *current_colors = normal_colors;
        for (size_t i = 0; i < params.size(); ++i) {
            int p = params[i];
            if (p == 0) {
                current_colors = normal_colors;
                current_attr = CharAttr(); // Light Gray on Black
            } else if (p == 1) {
                current_colors = bright_colors;
                current_attr.fg = bright_colors[7]; // Bright White, same background
            } else if (p >= 30 && p <= 37) {
                current_attr.fg = current_colors[p - 30];
            } else if (p >= 40 && p <= 47) {
                current_attr.bg = current_colors[p - 40];
            } else if (p >= 90 && p <= 97) {
                current_attr.fg = bright_colors[p - 90];
            } else if (p >= 100 && p <= 107) {
                current_attr.bg = bright_colors[p - 100];
            }
        }
        break;
    }
    case 'H':
        cursor.row = std::max(0, std::min(get_param(params, 0, 1) - 1, term_rows - 1));
        cursor.col = std::max(0, std::min(get_param(params, 1, 1) - 1, term_cols - 1));
        break;

    case 'A':
        cursor.row = std::max(0, cursor.row - get_param(params, 0, 1));
        break;

    case 'B':
        cursor.row = std::min(term_rows - 1, cursor.row + get_param(params, 0, 1));
        break;

    case 'C':
        cursor.col = std::min(term_cols - 1, cursor.col + get_param(params, 0, 1));
        break;

    case 'D':
        cursor.col = std::max(0, cursor.col - get_param(params, 0, 1));
        break;

    case 'J':
        switch (get_param(params, 0, 0)) {
        default:
        case 0:
            // Clear from cursor to end of screen
            for (int c = cursor.col; c < term_cols; ++c) {
                text_buffer[cursor.row][c] = { L' ', current_attr };
            }
            for (int r = cursor.row + 1; r < term_rows; ++r) {
                for (int c = 0; c < term_cols; ++c) {
                    text_buffer[r][c] = { L' ', current_attr };
                }
            }
            for (int r = cursor.row; r < term_rows; ++r) {
                dirty_rows.push_back(r);
            }
            break;
        case 1:
            // Clear from start of screen to cursor
            for (int r = 0; r < cursor.row; ++r) {
                for (int c = 0; c < term_cols; ++c) {
                    text_buffer[r][c] = { L' ', current_attr };
                }
            }
            for (int c = 0; c <= cursor.col; ++c) {
                text_buffer[cursor.row][c] = { L' ', current_attr };
            }
            for (int r = 0; r <= cursor.row; ++r) {
                dirty_rows.push_back(r);
            }
            break;
        case 2:
            clear_screen();
            for (int r = 0; r < term_rows; ++r) {
                dirty_rows.push_back(r);
            }
            break;
        }
        break;

    case 'K':
        // std::cerr << "Processing ESC [ " << mode << "K" << std::endl;
        switch (get_param(params, 0, 0)) {
        default:
        case 0:
            for (int c = cursor.col; c < term_cols; ++c) {
                text_buffer[cursor.row][c] = { L' ', current_attr };
            }
            break;
        case 1:
            for (int c = 0; c <= cursor.col; ++c) {
                text_buffer[cursor.row][c] = { L' ', current_attr };
            }
            break;
        case 2:
            for (int c = 0; c < term_cols; ++c) {
                text_buffer[cursor.row][c] = { L' ', current_attr };
            }
            break;
        }
        dirty_rows.push_back(cursor.row);
        break;
    }
}

void AnsiLogic::clear_screen()
{
    for (int r = 0; r < term_rows; ++r) {
        text_buffer[r] = std::vector<Char>(term_cols, { L' ', current_attr });
    }
    cursor.row = 0;
    cursor.col = 0;
}

void AnsiLogic::reset_state()
{
    // std::cerr << "Processing ESC c: Resetting terminal state" << std::endl;
    current_attr = CharAttr();
    clear_screen();
}

void AnsiLogic::scroll_up()
{
    text_buffer.erase(text_buffer.begin());
    text_buffer.push_back(std::vector<Char>(term_cols, { L' ', current_attr }));
    cursor.row = term_rows - 1;
}
