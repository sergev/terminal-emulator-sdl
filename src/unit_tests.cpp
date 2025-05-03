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

#include "ansi_logic.h"

// Test fixture for AnsiLogic
class AnsiLogicTest : public ::testing::Test {
protected:
    void SetUp() override { logic = std::make_unique<AnsiLogic>(80, 24); }

    std::unique_ptr<AnsiLogic> logic;
};

// Test ESC c (reset and clear screen)
TEST_F(AnsiLogicTest, EscCResetsStateAndClearsScreen)
{
    logic->current_attr.fg    = { 255, 0, 0 }; // Red foreground
    logic->cursor             = { 5, 10 };
    logic->text_buffer[5][10] = { L'x', logic->current_attr };

    std::vector<int> dirty_rows;
    logic->parse_ansi_sequence("c", dirty_rows);

    EXPECT_EQ(logic->current_attr.fg.r, 255);
    EXPECT_EQ(logic->current_attr.fg.g, 255);
    EXPECT_EQ(logic->current_attr.fg.b, 255);
    EXPECT_EQ(logic->cursor.row, 0);
    EXPECT_EQ(logic->cursor.col, 0);
    for (int r = 0; r < logic->get_rows(); ++r) {
        for (int c = 0; c < logic->get_cols(); ++c) {
            EXPECT_EQ(logic->text_buffer[r][c].ch, L' ');
        }
    }
    std::vector<int> expected_dirty_rows;
    for (int r = 0; r < logic->get_rows(); ++r) {
        expected_dirty_rows.push_back(r);
    }
    EXPECT_EQ(dirty_rows, expected_dirty_rows);
}

// Test ESC [ K (erase in line)
TEST_F(AnsiLogicTest, EscKClearsLine)
{
    logic->cursor = { 5, 10 };
    for (int c = 0; c < logic->get_cols(); ++c) {
        logic->text_buffer[5][c] = { L'x', logic->current_attr };
    }

    // Test mode 0: clear from cursor to end
    std::vector<int> dirty_rows;
    logic->parse_ansi_sequence("[0K", dirty_rows);
    for (int c = 0; c < 10; ++c) {
        EXPECT_EQ(logic->text_buffer[5][c].ch, L'x');
    }
    for (int c = 10; c < logic->get_cols(); ++c) {
        EXPECT_EQ(logic->text_buffer[5][c].ch, L' ');
    }
    EXPECT_EQ(dirty_rows, std::vector<int>({ 5 }));

    // Test mode 1: clear from start to cursor
    for (int c = 0; c < logic->get_cols(); ++c) {
        logic->text_buffer[5][c] = { L'x', logic->current_attr };
    }
    dirty_rows.clear();
    logic->parse_ansi_sequence("[1K", dirty_rows);
    for (int c = 0; c <= 10; ++c) {
        EXPECT_EQ(logic->text_buffer[5][c].ch, L' ');
    }
    for (int c = 11; c < logic->get_cols(); ++c) {
        EXPECT_EQ(logic->text_buffer[5][c].ch, L'x');
    }
    EXPECT_EQ(dirty_rows, std::vector<int>({ 5 }));

    // Test mode 2: clear entire line
    for (int c = 0; c < logic->get_cols(); ++c) {
        logic->text_buffer[5][c] = { L'x', logic->current_attr };
    }
    dirty_rows.clear();
    logic->parse_ansi_sequence("[2K", dirty_rows);
    for (int c = 0; c < logic->get_cols(); ++c) {
        EXPECT_EQ(logic->text_buffer[5][c].ch, L' ');
    }
    EXPECT_EQ(dirty_rows, std::vector<int>({ 5 }));
}

// Test ESC [ m (SGR)
TEST_F(AnsiLogicTest, EscMSetsColors)
{
    std::vector<int> dirty_rows;

    logic->parse_ansi_sequence("[31m", dirty_rows); // Red foreground
    EXPECT_EQ(logic->current_attr.fg.r, 255);
    EXPECT_EQ(logic->current_attr.fg.g, 0);
    EXPECT_EQ(logic->current_attr.fg.b, 0);
    EXPECT_TRUE(dirty_rows.empty());

    logic->parse_ansi_sequence("[41m", dirty_rows); // Red background
    EXPECT_EQ(logic->current_attr.fg.r, 255);       // Foreground should remain red
    EXPECT_EQ(logic->current_attr.fg.g, 0);
    EXPECT_EQ(logic->current_attr.fg.b, 0);
    EXPECT_EQ(logic->current_attr.bg.r, 255);
    EXPECT_EQ(logic->current_attr.bg.g, 0);
    EXPECT_EQ(logic->current_attr.bg.b, 0);
    EXPECT_TRUE(dirty_rows.empty());

    logic->parse_ansi_sequence("[0m", dirty_rows); // Reset
    EXPECT_EQ(logic->current_attr.fg.r, 255);
    EXPECT_EQ(logic->current_attr.fg.g, 255);
    EXPECT_EQ(logic->current_attr.fg.b, 255);
    EXPECT_EQ(logic->current_attr.bg.r, 0);
    EXPECT_EQ(logic->current_attr.bg.g, 0);
    EXPECT_EQ(logic->current_attr.bg.b, 0);
    EXPECT_TRUE(dirty_rows.empty());
}

// Test cursor movement
TEST_F(AnsiLogicTest, CursorMovement)
{
    logic->cursor = { 5, 10 };
    std::vector<int> dirty_rows;

    logic->parse_ansi_sequence("[3;5H", dirty_rows); // Move to row 3, col 5
    EXPECT_EQ(logic->cursor.row, 2);
    EXPECT_EQ(logic->cursor.col, 4);
    EXPECT_EQ(dirty_rows, std::vector<int>({ 2 }));

    dirty_rows.clear();
    logic->parse_ansi_sequence("[2A", dirty_rows); // Up 2
    EXPECT_EQ(logic->cursor.row, 0);
    EXPECT_EQ(logic->cursor.col, 4);
    EXPECT_EQ(dirty_rows, std::vector<int>({ 0 }));

    dirty_rows.clear();
    logic->parse_ansi_sequence("[3B", dirty_rows); // Down 3
    EXPECT_EQ(logic->cursor.row, 3);
    EXPECT_EQ(logic->cursor.col, 4);
    EXPECT_EQ(dirty_rows, std::vector<int>({ 3 }));

    dirty_rows.clear();
    logic->parse_ansi_sequence("[5C", dirty_rows); // Right 5
    EXPECT_EQ(logic->cursor.row, 3);
    EXPECT_EQ(logic->cursor.col, 9);
    EXPECT_EQ(dirty_rows, std::vector<int>({ 3 }));

    dirty_rows.clear();
    logic->parse_ansi_sequence("[2D", dirty_rows); // Left 2
    EXPECT_EQ(logic->cursor.row, 3);
    EXPECT_EQ(logic->cursor.col, 7);
    EXPECT_EQ(dirty_rows, std::vector<int>({ 3 }));
}

// Test Shift modifier
TEST_F(AnsiLogicTest, ShiftModifier)
{
    std::string input;

    input = logic->process_key(KeyInput('a', true, false)); // Shift+A
    EXPECT_EQ(input, "A");

    input = logic->process_key(KeyInput('1', true, false)); // Shift+1
    EXPECT_EQ(input, "!");
}

// Test Control modifier
TEST_F(AnsiLogicTest, ControlModifier)
{
    std::string input;

    input = logic->process_key(KeyInput('a', false, true)); // Ctrl+A
    EXPECT_EQ(input, std::string(1, 0x01));

    input = logic->process_key(KeyInput('z', false, true)); // Ctrl+Z
    EXPECT_EQ(input, std::string(1, 0x1A));
}

// Test text buffer insertion
TEST_F(AnsiLogicTest, TextBufferInsertion)
{
    logic->cursor             = { 5, 10 };
    logic->text_buffer[5][10] = { L'x', logic->current_attr };

    // Simulate printable character input
    logic->text_buffer[5][10] = { L'y', logic->current_attr };
    logic->cursor.col++;

    EXPECT_EQ(logic->text_buffer[5][10].ch, L'y');
    EXPECT_EQ(logic->cursor.col, 11);
}

// Test scroll up
TEST_F(AnsiLogicTest, ScrollUp)
{
    // Fill the first row with 'a'
    for (int c = 0; c < logic->get_cols(); ++c) {
        logic->text_buffer[0][c] = { L'a', logic->current_attr };
    }
    // Fill the last row with 'b'
    for (int c = 0; c < logic->get_cols(); ++c) {
        logic->text_buffer[logic->get_rows() - 1][c] = { L'b', logic->current_attr };
    }

    // Set cursor to last row
    logic->cursor.row = logic->get_rows() - 1;
    logic->cursor.col = 0;

    // Process a newline to trigger scroll
    const char input[] = "\n";
    auto dirty_rows    = logic->process_input(input, 1);

    // Verify buffer shifted: first row is gone, second row now first, last row is blank
    EXPECT_EQ(logic->text_buffer[0][0].ch, L' ');
    EXPECT_EQ(logic->text_buffer[logic->get_rows() - 2][0].ch, L'b');
    EXPECT_EQ(logic->text_buffer[logic->get_rows() - 1][0].ch, L' ');

    // Verify cursor is on the last row
    EXPECT_EQ(logic->cursor.row, logic->get_rows() - 1);
    EXPECT_EQ(logic->cursor.col, 0);

    // Verify all rows are marked dirty
    std::vector<int> expected_dirty_rows;
    for (int r = 0; r < logic->get_rows(); ++r) {
        expected_dirty_rows.push_back(r);
    }
    EXPECT_EQ(dirty_rows, expected_dirty_rows);
}

// Test ESC [0J (clear from cursor to end of screen)
TEST_F(AnsiLogicTest, ClearScreenEsc0J)
{
    // Fill buffer with 'x'
    for (int r = 0; r < logic->get_rows(); ++r) {
        for (int c = 0; c < logic->get_cols(); ++c) {
            logic->text_buffer[r][c] = { L'x', logic->current_attr };
        }
    }
    logic->cursor = { 5, 10 };

    // Process ESC [0J
    const char input[] = "\033[0J";
    auto dirty_rows    = logic->process_input(input, 4);

    // Verify rows before cursor.row are unchanged
    for (int r = 0; r < 5; ++r) {
        for (int c = 0; c < logic->get_cols(); ++c) {
            EXPECT_EQ(logic->text_buffer[r][c].ch, L'x');
        }
    }
    // Verify cursor.row from cursor.col to end is cleared
    for (int c = 0; c < 10; ++c) {
        EXPECT_EQ(logic->text_buffer[5][c].ch, L'x');
    }
    for (int c = 10; c < logic->get_cols(); ++c) {
        EXPECT_EQ(logic->text_buffer[5][c].ch, L' ');
    }
    // Verify rows after cursor.row are cleared
    for (int r = 6; r < logic->get_rows(); ++r) {
        for (int c = 0; c < logic->get_cols(); ++c) {
            EXPECT_EQ(logic->text_buffer[r][c].ch, L' ');
        }
    }

    // Verify cursor position unchanged
    EXPECT_EQ(logic->cursor.row, 5);
    EXPECT_EQ(logic->cursor.col, 10);

    // Verify dirty rows (from cursor.row to end)
    std::vector<int> expected_dirty_rows;
    for (int r = 5; r < logic->get_rows(); ++r) {
        expected_dirty_rows.push_back(r);
    }
    EXPECT_EQ(dirty_rows, expected_dirty_rows);
}

// Test ESC [1J (clear from start of screen to cursor)
TEST_F(AnsiLogicTest, ClearScreenEsc1J)
{
    // Fill buffer with 'x'
    for (int r = 0; r < logic->get_rows(); ++r) {
        for (int c = 0; c < logic->get_cols(); ++c) {
            logic->text_buffer[r][c] = { L'x', logic->current_attr };
        }
    }
    logic->cursor = { 5, 10 };

    // Process ESC [1J
    const char input[] = "\033[1J";
    auto dirty_rows    = logic->process_input(input, 4);

    // Verify rows before cursor.row are cleared
    for (int r = 0; r < 5; ++r) {
        for (int c = 0; c < logic->get_cols(); ++c) {
            EXPECT_EQ(logic->text_buffer[r][c].ch, L' ');
        }
    }
    // Verify cursor.row from start to cursor.col is cleared
    for (int c = 0; c <= 10; ++c) {
        EXPECT_EQ(logic->text_buffer[5][c].ch, L' ');
    }
    for (int c = 11; c < logic->get_cols(); ++c) {
        EXPECT_EQ(logic->text_buffer[5][c].ch, L'x');
    }
    // Verify rows after cursor.row are unchanged
    for (int r = 6; r < logic->get_rows(); ++r) {
        for (int c = 0; c < logic->get_cols(); ++c) {
            EXPECT_EQ(logic->text_buffer[r][c].ch, L'x');
        }
    }

    // Verify cursor position unchanged
    EXPECT_EQ(logic->cursor.row, 5);
    EXPECT_EQ(logic->cursor.col, 10);

    // Verify dirty rows (from 0 to cursor.row)
    std::vector<int> expected_dirty_rows;
    for (int r = 0; r <= 5; ++r) {
        expected_dirty_rows.push_back(r);
    }
    EXPECT_EQ(dirty_rows, expected_dirty_rows);
}

// Test ESC [2J (clear entire screen)
TEST_F(AnsiLogicTest, ClearScreenEsc2J)
{
    // Fill buffer with 'x'
    for (int r = 0; r < logic->get_rows(); ++r) {
        for (int c = 0; c < logic->get_cols(); ++c) {
            logic->text_buffer[r][c] = { L'x', logic->current_attr };
        }
    }
    logic->cursor = { 5, 10 };

    // Process ESC [2J
    const char input[] = "\033[2J";
    auto dirty_rows    = logic->process_input(input, 4);

    // Verify entire buffer is cleared
    for (int r = 0; r < logic->get_rows(); ++r) {
        for (int c = 0; c < logic->get_cols(); ++c) {
            EXPECT_EQ(logic->text_buffer[r][c].ch, L' ');
        }
    }

    // Verify cursor is at (0,0)
    EXPECT_EQ(logic->cursor.row, 0);
    EXPECT_EQ(logic->cursor.col, 0);

    // Verify all rows are marked dirty
    std::vector<int> expected_dirty_rows;
    for (int r = 0; r < logic->get_rows(); ++r) {
        expected_dirty_rows.push_back(r);
    }
    EXPECT_EQ(dirty_rows, expected_dirty_rows);
}

// Test UTF-8 input decoding
TEST_F(AnsiLogicTest, Utf8Input)
{
    // Test ASCII
    const char ascii[] = "a";
    logic->cursor      = { 5, 10 };
    logic->process_input(ascii, 1);
    EXPECT_EQ(logic->text_buffer[5][10].ch, L'a');

    // Test 2-byte UTF-8 (Cyrillic 'Я')
    const char cyrillic[] = "\xD0\xAF";
    logic->cursor         = { 5, 11 };
    logic->process_input(cyrillic, 2);
    EXPECT_EQ(logic->text_buffer[5][11].ch, 0x042F); // Я

    // Test 3-byte UTF-8 (Euro symbol '€')
    const char euro[] = "\xE2\x82\xAC";
    logic->cursor     = { 5, 12 };
    logic->process_input(euro, 3);
    EXPECT_EQ(logic->text_buffer[5][12].ch, 0x20AC); // €

    // Test 4-byte UTF-8 (emoji '😀')
    const char emoji[] = "\xF0\x9F\x98\x80";
    logic->cursor      = { 5, 13 };
    logic->process_input(emoji, 4);
    EXPECT_EQ(logic->text_buffer[5][13].ch, 0x1F600); // 😀
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
