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
#include "terminal_logic.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <unicode/uchar.h>

const CharAttr TerminalLogic::ansi_colors[] = {
    { 0, 0, 0, 255, 0, 0, 0, 255 },       // Black
    { 255, 0, 0, 255, 0, 0, 0, 255 },     // Red
    { 0, 255, 0, 255, 0, 0, 0, 255 },     // Green
    { 255, 255, 0, 255, 0, 0, 0, 255 },   // Yellow
    { 0, 0, 255, 255, 0, 0, 0, 255 },     // Blue
    { 255, 0, 255, 255, 0, 0, 0, 255 },   // Magenta
    { 0, 255, 255, 255, 0, 0, 0, 255 },   // Cyan
    { 255, 255, 255, 255, 0, 0, 0, 255 }, // White
};

TerminalLogic::TerminalLogic(int cols, int rows)
    : term_cols(cols), term_rows(rows), state(AnsiState::NORMAL)
{
    text_buffer.resize(term_rows, std::vector<Char>(term_cols, { ' ', current_attr }));
}

void TerminalLogic::resize(int new_cols, int new_rows)
{
    term_cols = new_cols;
    term_rows = new_rows;
    text_buffer.resize(term_rows, std::vector<Char>(term_cols, { ' ', current_attr }));
    for (auto &line : text_buffer) {
        line.resize(term_cols, { ' ', current_attr });
    }
    cursor.row = std::min(cursor.row, term_rows - 1);
    cursor.col = std::min(cursor.col, term_cols - 1);
}

std::vector<int> TerminalLogic::process_input(const char *buffer, size_t length)
{
    std::vector<int> dirty_rows;
    size_t i = 0;
    while (i < length) {
        char c = buffer[i];
        switch (state) {
        case AnsiState::NORMAL:
            if (c == '\033') {
                state = AnsiState::ESCAPE;
                ansi_seq.clear();
                // std::cerr << "Received ESC, transitioning to ESCAPE state" << std::endl;
                ++i;
            } else if (c == '\n') {
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
            } else if (c == '\r') {
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
            } else if (c == '\b') {
                if (cursor.col > 0) {
                    cursor.col--;
                    text_buffer[cursor.row][cursor.col] = { L' ', current_attr };
                    dirty_rows.push_back(cursor.row);
                }
                ++i;
            } else {
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
            if (c == '[') {
                state = AnsiState::CSI;
                ansi_seq.clear();
                ansi_seq += c;
                // std::cerr << "Received [, transitioning to CSI state" << std::endl;
            } else if (c == 'c') {
                // std::cerr << "Received ESC c, processing reset" << std::endl;
                parse_ansi_sequence("", c);
                state = AnsiState::NORMAL;
                ansi_seq.clear();
                for (int r = 0; r < term_rows; ++r) {
                    dirty_rows.push_back(r);
                }
            } else {
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
                parse_ansi_sequence(ansi_seq, c);
                state = AnsiState::NORMAL;
                ansi_seq.clear();
                dirty_rows.push_back(cursor.row);
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
    } // Add handling for wc > 0xFFFF if needed
    return utf8;
}

std::string TerminalLogic::process_key(const KeyInput &key)
{
    std::string input;
    // std::cerr << "Key processed: Keycode=" << key.code << ", Modifiers=" << modifiers <<
    // std::endl;

    // Map SDL keycodes to terminal inputs
    switch (key.code) {
    case KeyCode::UNKNOWN:
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
        input = "\033[11~";
        break;
    case KeyCode::F2:
        input = "\033[12~";
        break;
    case KeyCode::F3:
        input = "\033[13~";
        break;
    case KeyCode::F4:
        input = "\033[14~";
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
            if (key.character >= 'a' && key.character <= 'z') {
                char ctrl_char = (key.character - 'a') + 1;
                input          = std::string(1, ctrl_char);
            }
        } else if (key.character >= 'a' && key.character <= 'z') {
            char base_char = key.character;
            if (key.mod_shift) {
                base_char = std::toupper(base_char);
            }
            input = std::string(1, base_char);
        } else if (key.character >= ' ' && key.character <= '~') {
            char base_char = key.character;
            if (key.mod_shift) {
                static const std::map<char, char> shift_map = {
                    { '1', '!' },  { '2', '@' }, { '3', '#' }, { '4', '$' }, { '5', '%' },
                    { '6', '^' },  { '7', '&' }, { '8', '*' }, { '9', '(' }, { '0', ')' },
                    { '-', '_' },  { '=', '+' }, { '[', '{' }, { ']', '}' }, { ';', ':' },
                    { '\'', '"' }, { ',', '<' }, { '.', '>' }, { '/', '?' }, { '`', '~' }
                };
                auto it = shift_map.find(base_char);
                if (it != shift_map.end()) {
                    base_char = it->second;
                }
            }
            input = std::string(1, base_char);
        } else {
            auto ch = key.character;
            if (key.mod_shift) {
                // Convert Unicode character to uppercase
                ch = u_toupper(ch);
            }
            input = wchar_to_utf8(ch);
        }
        break;
    }
    return input;
}

const std::vector<std::vector<Char>> &TerminalLogic::get_text_buffer() const
{
    return text_buffer;
}

const Cursor &TerminalLogic::get_cursor() const
{
    return cursor;
}

void TerminalLogic::parse_ansi_sequence(const std::string &seq, char final_char)
{
    if (final_char == 'c') {
        reset_state();
        return;
    }

    if (seq.empty() || seq[0] != '[') {
        // std::cerr << "Invalid CSI sequence: " << seq << final_char << std::endl;
        return;
    }

    // std::cerr << "Processing CSI sequence: " << seq << final_char << std::endl;

    std::vector<int> params;
    std::string param_str;

    for (size_t i = 1; i < seq.size() - 1; ++i) {
        if (std::isdigit(seq[i])) {
            param_str += seq[i];
        } else if (seq[i] == ';' || i == seq.size() - 2) {
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
        }
    }
    if (!param_str.empty()) {
        try {
            params.push_back(std::stoi(param_str));
        } catch (const std::exception &e) {
            // std::cerr << "Error parsing final parameter '" << param_str << "': " << e.what()
            //           << std::endl;
            params.push_back(0);
        }
    }

    handle_csi_sequence(seq, final_char, params);
}

void TerminalLogic::handle_csi_sequence(const std::string &seq, char final_char,
                                        const std::vector<int> &params)
{
    switch (final_char) {
    case 'm':
        for (size_t i = 0; i < params.size(); ++i) {
            int p = params[i];
            if (p == 0) {
                current_attr = CharAttr();
            } else if (p >= 30 && p <= 37) {
                current_attr.fg_r = ansi_colors[p - 30].fg_r;
                current_attr.fg_g = ansi_colors[p - 30].fg_g;
                current_attr.fg_b = ansi_colors[p - 30].fg_b;
            } else if (p >= 40 && p <= 47) {
                current_attr.bg_r = ansi_colors[p - 40].bg_r;
                current_attr.bg_g = ansi_colors[p - 40].bg_g;
                current_attr.bg_b = ansi_colors[p - 40].bg_b;
            } else if (p >= 90 && p <= 97) {
                current_attr.fg_r = ansi_colors[p - 90].fg_r;
                current_attr.fg_g = ansi_colors[p - 90].fg_g;
                current_attr.fg_b = ansi_colors[p - 90].fg_b;
            } else if (p >= 100 && p <= 107) {
                current_attr.bg_r = ansi_colors[p - 100].bg_r;
                current_attr.bg_g = ansi_colors[p - 100].bg_g;
                current_attr.bg_b = ansi_colors[p - 100].bg_b;
            }
        }
        break;

    case 'H': {
        int row    = (params.size() > 0 ? params[0] : 1) - 1;
        int col    = (params.size() > 1 ? params[1] : 1) - 1;
        cursor.row = std::max(0, std::min(row, term_rows - 1));
        cursor.col = std::max(0, std::min(col, term_cols - 1));
        break;
    }

    case 'A':
        cursor.row = std::max(0, cursor.row - (params.empty() ? 1 : params[0]));
        break;

    case 'B':
        cursor.row = std::min(term_rows - 1, cursor.row + (params.empty() ? 1 : params[0]));
        break;

    case 'C':
        cursor.col = std::min(term_cols - 1, cursor.col + (params.empty() ? 1 : params[0]));
        break;

    case 'D':
        cursor.col = std::max(0, cursor.col - (params.empty() ? 1 : params[0]));
        break;

    case 'J': {
        int mode = params.empty() ? 0 : params[0];
        if (mode == 0) {
            // Clear from cursor to end of screen
            for (int c = cursor.col; c < term_cols; ++c) {
                text_buffer[cursor.row][c] = { ' ', current_attr };
            }
            for (int r = cursor.row + 1; r < term_rows; ++r) {
                for (int c = 0; c < term_cols; ++c) {
                    text_buffer[r][c] = { ' ', current_attr };
                }
            }
        } else if (mode == 1) {
            // Clear from start of screen to cursor
            for (int r = 0; r < cursor.row; ++r) {
                for (int c = 0; c < term_cols; ++c) {
                    text_buffer[r][c] = { ' ', current_attr };
                }
            }
            for (int c = 0; c <= cursor.col; ++c) {
                text_buffer[cursor.row][c] = { ' ', current_attr };
            }
        } else if (mode == 2) {
            clear_screen();
        }
        break;
    }

    case 'K': {
        int mode = params.empty() ? 0 : params[0];
        // std::cerr << "Processing ESC [ " << mode << "K" << std::endl;
        switch (mode) {
        case 0:
            for (int c = cursor.col; c < term_cols; ++c) {
                text_buffer[cursor.row][c] = { ' ', current_attr };
            }
            break;
        case 1:
            for (int c = 0; c <= cursor.col; ++c) {
                text_buffer[cursor.row][c] = { ' ', current_attr };
            }
            break;
        case 2:
            for (int c = 0; c < term_cols; ++c) {
                text_buffer[cursor.row][c] = { ' ', current_attr };
            }
            break;
        default:
            // std::cerr << "Unknown EL mode: " << mode << std::endl;
            break;
        }
        break;
    }
    }
}

void TerminalLogic::clear_screen()
{
    for (int r = 0; r < term_rows; ++r) {
        text_buffer[r] = std::vector<Char>(term_cols, { ' ', current_attr });
    }
    cursor.row = 0;
    cursor.col = 0;
}

void TerminalLogic::reset_state()
{
    // std::cerr << "Processing ESC c: Resetting terminal state" << std::endl;
    current_attr = CharAttr();
    clear_screen();
}

void TerminalLogic::scroll_up()
{
    text_buffer.erase(text_buffer.begin());
    text_buffer.push_back(std::vector<Char>(term_cols, { ' ', current_attr }));
    cursor.row = term_rows - 1;
}
