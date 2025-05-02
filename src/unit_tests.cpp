//
// Unit tests for the terminal emulator.
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
#include <gtest/gtest.h>

#include "terminal_logic.h"

// Test fixture for TerminalLogic
class TerminalLogicTest : public ::testing::Test {
protected:
    void SetUp() override { logic = std::make_unique<TerminalLogic>(80, 24); }

    std::unique_ptr<TerminalLogic> logic;
};

// Test ESC c (reset and clear screen)
TEST_F(TerminalLogicTest, EscCResetsStateAndClearsScreen)
{
    logic->current_attr.fg_r  = 255;
    logic->current_attr.fg_g  = 0;
    logic->current_attr.fg_b  = 0; // Red foreground
    logic->cursor             = { 5, 10 };
    logic->text_buffer[5][10] = { 'x', logic->current_attr };

    logic->parse_ansi_sequence("", 'c');

    EXPECT_EQ(logic->current_attr.fg_r, 255);
    EXPECT_EQ(logic->current_attr.fg_g, 255);
    EXPECT_EQ(logic->current_attr.fg_b, 255);
    EXPECT_EQ(logic->cursor.row, 0);
    EXPECT_EQ(logic->cursor.col, 0);
    for (int r = 0; r < logic->term_rows; ++r) {
        for (int c = 0; c < logic->term_cols; ++c) {
            EXPECT_EQ(logic->text_buffer[r][c].ch, ' ');
        }
    }
}

// Test ESC [ K (erase in line)
TEST_F(TerminalLogicTest, EscKClearsLine)
{
    logic->cursor = { 5, 10 };
    for (int c = 0; c < logic->term_cols; ++c) {
        logic->text_buffer[5][c] = { 'x', logic->current_attr };
    }

    // Test mode 0: clear from cursor to end
    logic->parse_ansi_sequence("[0", 'K');
    for (int c = 0; c < 10; ++c) {
        EXPECT_EQ(logic->text_buffer[5][c].ch, 'x');
    }
    for (int c = 10; c < logic->term_cols; ++c) {
        EXPECT_EQ(logic->text_buffer[5][c].ch, ' ');
    }

    // Test mode 1: clear from start to cursor
    for (int c = 0; c < logic->term_cols; ++c) {
        logic->text_buffer[5][c] = { 'x', logic->current_attr };
    }
    logic->parse_ansi_sequence("[1", 'K');
    for (int c = 0; c <= 10; ++c) {
        EXPECT_EQ(logic->text_buffer[5][c].ch, ' ');
    }
    for (int c = 11; c < logic->term_cols; ++c) {
        EXPECT_EQ(logic->text_buffer[5][c].ch, 'x');
    }

    // Test mode 2: clear entire line
    for (int c = 0; c < logic->term_cols; ++c) {
        logic->text_buffer[5][c] = { 'x', logic->current_attr };
    }
    logic->parse_ansi_sequence("[2", 'K');
    for (int c = 0; c < logic->term_cols; ++c) {
        EXPECT_EQ(logic->text_buffer[5][c].ch, ' ');
    }
}

// Test ESC [ m (SGR)
TEST_F(TerminalLogicTest, EscMSetsColors)
{
    logic->parse_ansi_sequence("[31", 'm'); // Red foreground
    EXPECT_EQ(logic->current_attr.fg_r, 255);
    EXPECT_EQ(logic->current_attr.fg_g, 0);
    EXPECT_EQ(logic->current_attr.fg_b, 0);

    logic->parse_ansi_sequence("[41", 'm'); // Red background
    EXPECT_EQ(logic->current_attr.bg_r, 255);
    EXPECT_EQ(logic->current_attr.bg_g, 0);
    EXPECT_EQ(logic->current_attr.bg_b, 0);

    logic->parse_ansi_sequence("[0", 'm'); // Reset
    EXPECT_EQ(logic->current_attr.fg_r, 255);
    EXPECT_EQ(logic->current_attr.fg_g, 255);
    EXPECT_EQ(logic->current_attr.fg_b, 255);
    EXPECT_EQ(logic->current_attr.bg_r, 0);
    EXPECT_EQ(logic->current_attr.bg_g, 0);
    EXPECT_EQ(logic->current_attr.bg_b, 0);
}

// Test cursor movement
TEST_F(TerminalLogicTest, CursorMovement)
{
    logic->cursor = { 5, 10 };

    logic->parse_ansi_sequence("[3;5", 'H'); // Move to row 3, col 5
    EXPECT_EQ(logic->cursor.row, 2);
    EXPECT_EQ(logic->cursor.col, 4);

    logic->parse_ansi_sequence("[2", 'A'); // Up 2
    EXPECT_EQ(logic->cursor.row, 0);
    EXPECT_EQ(logic->cursor.col, 4);

    logic->parse_ansi_sequence("[3", 'B'); // Down 3
    EXPECT_EQ(logic->cursor.row, 3);
    EXPECT_EQ(logic->cursor.col, 4);

    logic->parse_ansi_sequence("[5", 'C'); // Right 5
    EXPECT_EQ(logic->cursor.row, 3);
    EXPECT_EQ(logic->cursor.col, 9);

    logic->parse_ansi_sequence("[2", 'D'); // Left 2
    EXPECT_EQ(logic->cursor.row, 3);
    EXPECT_EQ(logic->cursor.col, 7);
}

// Test Shift modifier
TEST_F(TerminalLogicTest, ShiftModifier)
{
    std::string input;

    input = logic->process_key('a', true, false); // Shift+A
    EXPECT_EQ(input, "A");

    input = logic->process_key('1', true, false); // Shift+1
    EXPECT_EQ(input, "!");
}

// Test Control modifier
TEST_F(TerminalLogicTest, ControlModifier)
{
    std::string input;

    input = logic->process_key('a', false, true); // Ctrl+A
    EXPECT_EQ(input, std::string(1, 0x01));

    input = logic->process_key('z', false, true); // Ctrl+Z
    EXPECT_EQ(input, std::string(1, 0x1A));
}

// Test text buffer insertion
TEST_F(TerminalLogicTest, TextBufferInsertion)
{
    logic->cursor             = { 5, 10 };
    logic->text_buffer[5][10] = { 'x', logic->current_attr };

    // Simulate printable character input
    logic->text_buffer[5][10] = { 'y', logic->current_attr };
    logic->cursor.col++;

    EXPECT_EQ(logic->text_buffer[5][10].ch, 'y');
    EXPECT_EQ(logic->cursor.col, 11);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
