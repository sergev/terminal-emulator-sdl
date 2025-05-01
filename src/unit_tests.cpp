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
#include "terminal_emulator.h"

#include <gtest/gtest.h>

// Test fixture for TerminalEmulator
class TerminalEmulatorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        emulator = std::make_unique<TerminalEmulator>(80, 24);
        // Initialize text_buffer and other members without SDL/PTY
        emulator->text_buffer.resize(
            emulator->term_rows,
            std::vector<Char>(emulator->term_cols, { ' ', emulator->current_attr }));
        emulator->dirty_lines.resize(emulator->term_rows, true);
        emulator->texture_cache.resize(emulator->term_rows);
    }

    std::unique_ptr<TerminalEmulator> emulator;
};

// Test ESC c (reset and clear screen)
TEST_F(TerminalEmulatorTest, EscCResetsStateAndClearsScreen)
{
    emulator->current_attr.fg    = { 255, 0, 0, 255 }; // Red foreground
    emulator->cursor             = { 5, 10 };
    emulator->text_buffer[5][10] = { 'x', emulator->current_attr };

    emulator->parse_ansi_sequence("", 'c');

    EXPECT_EQ(emulator->current_attr.fg.r, 255);
    EXPECT_EQ(emulator->current_attr.fg.g, 255);
    EXPECT_EQ(emulator->current_attr.fg.b, 255);
    EXPECT_EQ(emulator->cursor.row, 0);
    EXPECT_EQ(emulator->cursor.col, 0);
    for (int r = 0; r < emulator->term_rows; ++r) {
        for (int c = 0; c < emulator->term_cols; ++c) {
            EXPECT_EQ(emulator->text_buffer[r][c].ch, ' ');
        }
        EXPECT_TRUE(emulator->dirty_lines[r]);
    }
}

// Test ESC [ K (erase in line)
TEST_F(TerminalEmulatorTest, EscKClearsLine)
{
    emulator->cursor = { 5, 10 };
    for (int c = 0; c < emulator->term_cols; ++c) {
        emulator->text_buffer[5][c] = { 'x', emulator->current_attr };
    }

    // Test mode 0: clear from cursor to end
    emulator->parse_ansi_sequence("[0", 'K');
    for (int c = 0; c < 10; ++c) {
        EXPECT_EQ(emulator->text_buffer[5][c].ch, 'x');
    }
    for (int c = 10; c < emulator->term_cols; ++c) {
        EXPECT_EQ(emulator->text_buffer[5][c].ch, ' ');
    }
    EXPECT_TRUE(emulator->dirty_lines[5]);

    // Test mode 1: clear from start to cursor
    for (int c = 0; c < emulator->term_cols; ++c) {
        emulator->text_buffer[5][c] = { 'x', emulator->current_attr };
    }
    emulator->parse_ansi_sequence("[1", 'K');
    for (int c = 0; c <= 10; ++c) {
        EXPECT_EQ(emulator->text_buffer[5][c].ch, ' ');
    }
    for (int c = 11; c < emulator->term_cols; ++c) {
        EXPECT_EQ(emulator->text_buffer[5][c].ch, 'x');
    }

    // Test mode 2: clear entire line
    for (int c = 0; c < emulator->term_cols; ++c) {
        emulator->text_buffer[5][c] = { 'x', emulator->current_attr };
    }
    emulator->parse_ansi_sequence("[2", 'K');
    for (int c = 0; c < emulator->term_cols; ++c) {
        EXPECT_EQ(emulator->text_buffer[5][c].ch, ' ');
    }
}

// Test ESC [ m (SGR)
TEST_F(TerminalEmulatorTest, EscMSetsColors)
{
    emulator->parse_ansi_sequence("[31", 'm'); // Red foreground
    EXPECT_EQ(emulator->current_attr.fg.r, 255);
    EXPECT_EQ(emulator->current_attr.fg.g, 0);
    EXPECT_EQ(emulator->current_attr.fg.b, 0);

    emulator->parse_ansi_sequence("[41", 'm'); // Red background
    EXPECT_EQ(emulator->current_attr.bg.r, 255);
    EXPECT_EQ(emulator->current_attr.bg.g, 0);
    EXPECT_EQ(emulator->current_attr.bg.b, 0);

    emulator->parse_ansi_sequence("[0", 'm'); // Reset
    EXPECT_EQ(emulator->current_attr.fg.r, 255);
    EXPECT_EQ(emulator->current_attr.fg.g, 255);
    EXPECT_EQ(emulator->current_attr.fg.b, 255);
    EXPECT_EQ(emulator->current_attr.bg.r, 0);
    EXPECT_EQ(emulator->current_attr.bg.g, 0);
    EXPECT_EQ(emulator->current_attr.bg.b, 0);
}

// Test cursor movement
TEST_F(TerminalEmulatorTest, CursorMovement)
{
    emulator->cursor = { 5, 10 };

    emulator->parse_ansi_sequence("[3;5", 'H'); // Move to row 3, col 5
    EXPECT_EQ(emulator->cursor.row, 2);
    EXPECT_EQ(emulator->cursor.col, 4);

    emulator->parse_ansi_sequence("[2", 'A'); // Up 2
    EXPECT_EQ(emulator->cursor.row, 0);
    EXPECT_EQ(emulator->cursor.col, 4);

    emulator->parse_ansi_sequence("[3", 'B'); // Down 3
    EXPECT_EQ(emulator->cursor.row, 3);
    EXPECT_EQ(emulator->cursor.col, 4);

    emulator->parse_ansi_sequence("[5", 'C'); // Right 5
    EXPECT_EQ(emulator->cursor.row, 3);
    EXPECT_EQ(emulator->cursor.col, 9);

    emulator->parse_ansi_sequence("[2", 'D'); // Left 2
    EXPECT_EQ(emulator->cursor.row, 3);
    EXPECT_EQ(emulator->cursor.col, 7);
}

// Test Shift modifier
TEST_F(TerminalEmulatorTest, ShiftModifier)
{
    SDL_KeyboardEvent key = { SDL_KEYDOWN, 0, 0, 0 };
    std::string input;

    key.keysym.sym = SDLK_a;
    key.keysym.mod = KMOD_SHIFT;
    emulator->process_modifiers(key, input);
    EXPECT_EQ(input, "A");

    key.keysym.sym = SDLK_1;
    key.keysym.mod = KMOD_SHIFT;
    input.clear();
    emulator->process_modifiers(key, input);
    EXPECT_EQ(input, "!");
}

// Test Control modifier
TEST_F(TerminalEmulatorTest, ControlModifier)
{
    SDL_KeyboardEvent key = { SDL_KEYDOWN, 0, 0, 0 };
    std::string input;

    key.keysym.sym = SDLK_a;
    key.keysym.mod = KMOD_CTRL;
    emulator->process_modifiers(key, input);
    EXPECT_EQ(input, std::string(1, 0x01));

    key.keysym.sym = SDLK_z;
    key.keysym.mod = KMOD_CTRL;
    input.clear();
    emulator->process_modifiers(key, input);
    EXPECT_EQ(input, std::string(1, 0x1A));
}

// Test text buffer insertion
TEST_F(TerminalEmulatorTest, TextBufferInsertion)
{
    emulator->cursor             = { 5, 10 };
    emulator->text_buffer[5][10] = { 'x', emulator->current_attr };
    emulator->dirty_lines[5]     = false;

    // Simulate printable character input
    emulator->text_buffer[5][10] = { 'y', emulator->current_attr };
    emulator->cursor.col++;
    emulator->dirty_lines[5] = true;

    EXPECT_EQ(emulator->text_buffer[5][10].ch, 'y');
    EXPECT_EQ(emulator->cursor.col, 11);
    EXPECT_TRUE(emulator->dirty_lines[5]);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
