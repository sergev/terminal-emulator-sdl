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
#include "system_interface.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#include <algorithm>
#include <iostream>

SystemInterface::SystemInterface(int cols, int rows) : terminal(cols, rows), dirty_lines(rows, true)
{
    initialize_sdl();
    initialize_pty();
    texture_cache.resize(rows);
}

SystemInterface::~SystemInterface()
{
    if (child_pid > 0) {
        kill(child_pid, SIGTERM);
        waitpid(child_pid, nullptr, 0);
    }
    if (pty_fd != -1) {
        close(pty_fd);
    }
    if (font) {
        TTF_CloseFont(static_cast<TTF_Font *>(font));
    }
    if (renderer) {
        SDL_DestroyRenderer(static_cast<SDL_Renderer *>(renderer));
    }
    if (window) {
        SDL_DestroyWindow(static_cast<SDL_Window *>(window));
    }
    TTF_Quit();
    SDL_Quit();
}

void SystemInterface::initialize_sdl()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        exit(1);
    }
    if (TTF_Init() < 0) {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        exit(1);
    }

    window = SDL_CreateWindow("Terminal Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        exit(1);
    }

    renderer = SDL_CreateRenderer(static_cast<SDL_Window *>(window), -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        exit(1);
    }

    const char *font_path = "/System/Library/Fonts/Menlo.ttc";
    font                  = TTF_OpenFont(font_path, font_size);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        exit(1);
    }
}

void SystemInterface::initialize_pty()
{
    pty_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (pty_fd < 0) {
        std::cerr << "posix_openpt failed: " << strerror(errno) << std::endl;
        exit(1);
    }
    if (grantpt(pty_fd) < 0 || unlockpt(pty_fd) < 0) {
        std::cerr << "PTY setup failed: " << strerror(errno) << std::endl;
        exit(1);
    }

    child_pid = fork();
    if (child_pid < 0) {
        std::cerr << "fork failed: " << strerror(errno) << std::endl;
        exit(1);
    } else if (child_pid == 0) {
        const char *pty_name = ptsname(pty_fd);
        int slave_fd         = open(pty_name, O_RDWR);
        if (slave_fd < 0) {
            std::cerr << "open slave PTY failed: " << strerror(errno) << std::endl;
            exit(1);
        }

        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);
        close(slave_fd);
        close(pty_fd);

        setenv("TERM", "xterm-256color", 1);
        execlp("bash", "bash", nullptr);
        std::cerr << "execlp failed: " << strerror(errno) << std::endl;
        exit(1);
    }

    struct winsize ws = {};
    ws.ws_col         = terminal.get_cols();
    ws.ws_row         = terminal.get_rows();
    if (ioctl(pty_fd, TIOCSWINSZ, &ws) < 0) {
        std::cerr << "ioctl TIOCSWINSZ failed: " << strerror(errno) << std::endl;
    }
}

void SystemInterface::process_pty_input()
{
    char buffer[1024];
    ssize_t bytes_read = read(pty_fd, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        std::vector<int> dirty_rows = terminal.process_input(buffer, bytes_read);
        for (int row : dirty_rows) {
            if (row >= 0 && static_cast<size_t>(row) < dirty_lines.size()) {
                dirty_lines[row] = true;
            }
        }
    }
}

void SystemInterface::process_sdl_event()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            exit(0);
            break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                int width          = event.window.data1;
                int height         = event.window.data2;
                TTF_Font *ttf_font = static_cast<TTF_Font *>(font);
                int char_width, char_height;
                TTF_SizeText(ttf_font, "A", &char_width, &char_height);
                int new_cols = std::max(1, width / char_width);
                int new_rows = std::max(1, height / char_height);
                resize(new_cols, new_rows);
            }
            break;
        case SDL_KEYDOWN: {
            KeyCode keycode    = KeyCode::CHARACTER;
            char character     = 0;
            uint16_t modifiers = event.key.keysym.mod;

            // Map SDL2 keycodes to KeyCode enum
            switch (event.key.keysym.sym) {
            case SDLK_RETURN:
                keycode = KeyCode::RETURN;
                break;
            case SDLK_BACKSPACE:
                keycode = KeyCode::BACKSPACE;
                break;
            case SDLK_TAB:
                keycode = KeyCode::TAB;
                break;
            case SDLK_UP:
                keycode = KeyCode::UP;
                break;
            case SDLK_DOWN:
                keycode = KeyCode::DOWN;
                break;
            case SDLK_RIGHT:
                keycode = KeyCode::RIGHT;
                break;
            case SDLK_LEFT:
                keycode = KeyCode::LEFT;
                break;
            case SDLK_HOME:
                keycode = KeyCode::HOME;
                break;
            case SDLK_END:
                keycode = KeyCode::END;
                break;
            case SDLK_INSERT:
                keycode = KeyCode::INSERT;
                break;
            case SDLK_DELETE:
                keycode = KeyCode::DELETE;
                break;
            case SDLK_PAGEUP:
                keycode = KeyCode::PAGEUP;
                break;
            case SDLK_PAGEDOWN:
                keycode = KeyCode::PAGEDOWN;
                break;
            case SDLK_F1:
                keycode = KeyCode::F1;
                break;
            case SDLK_F2:
                keycode = KeyCode::F2;
                break;
            case SDLK_F3:
                keycode = KeyCode::F3;
                break;
            case SDLK_F4:
                keycode = KeyCode::F4;
                break;
            case SDLK_F5:
                keycode = KeyCode::F5;
                break;
            case SDLK_F6:
                keycode = KeyCode::F6;
                break;
            case SDLK_F7:
                keycode = KeyCode::F7;
                break;
            case SDLK_F8:
                keycode = KeyCode::F8;
                break;
            case SDLK_F9:
                keycode = KeyCode::F9;
                break;
            case SDLK_F10:
                keycode = KeyCode::F10;
                break;
            case SDLK_F11:
                keycode = KeyCode::F11;
                break;
            case SDLK_F12:
                keycode = KeyCode::F12;
                break;
            default:
                if (event.key.keysym.sym >= 32 && event.key.keysym.sym <= 126) {
                    character = static_cast<char>(event.key.keysym.sym);
                    keycode   = KeyCode::CHARACTER;
                }
                break;
            }

            // Process the key with the mapped KeyCode and character
            std::string input = terminal.process_key(keycode, modifiers, character);
            if (!input.empty()) {
                write(pty_fd, input.c_str(), input.size());
            }

            // Handle font size adjustment
            if (keycode == KeyCode::CHARACTER && modifiers & KMOD_GUI) {
                if (character == '=' || character == '+') {
                    font_size = std::min(72, font_size + 2);
                    TTF_CloseFont(static_cast<TTF_Font *>(font));
                    font = TTF_OpenFont("/System/Library/Fonts/Menlo.ttc", font_size);
                    if (!font) {
                        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
                        exit(1);
                    }
                    clear_texture_cache();
                    int char_width, char_height;
                    TTF_SizeText(static_cast<TTF_Font *>(font), "A", &char_width, &char_height);
                    int new_cols = std::max(1, 800 / char_width);
                    int new_rows = std::max(1, 600 / char_height);
                    resize(new_cols, new_rows);
                } else if (character == '-') {
                    font_size = std::max(8, font_size - 2);
                    TTF_CloseFont(static_cast<TTF_Font *>(font));
                    font = TTF_OpenFont("/System/Library/Fonts/Menlo.ttc", font_size);
                    if (!font) {
                        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
                        exit(1);
                    }
                    clear_texture_cache();
                    int char_width, char_height;
                    TTF_SizeText(static_cast<TTF_Font *>(font), "A", &char_width, &char_height);
                    int new_cols = std::max(1, 800 / char_width);
                    int new_rows = std::max(1, 600 / char_height);
                    resize(new_cols, new_rows);
                }
            }
            break;
        }
        }
    }
}

void SystemInterface::render_frame()
{
    update_texture_cache();
    SDL_Renderer *sdl_renderer = static_cast<SDL_Renderer *>(renderer);
    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
    SDL_RenderClear(sdl_renderer);

    TTF_Font *ttf_font = static_cast<TTF_Font *>(font);
    int char_width, char_height;
    TTF_SizeText(ttf_font, "A", &char_width, &char_height);

    for (size_t row = 0; row < texture_cache.size(); ++row) {
        render_text(row, texture_cache[row]);
    }

    // Render cursor
    const Cursor &cursor = terminal.get_cursor();
    if (cursor.row >= 0 && cursor.row < static_cast<int>(texture_cache.size()) && cursor.col >= 0 &&
        cursor.col < terminal.get_cols()) {
        SDL_Rect cursor_rect = { cursor.col * char_width, cursor.row * char_height, char_width,
                                 char_height };
        SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(sdl_renderer, &cursor_rect);
    }

    SDL_RenderPresent(sdl_renderer);
}

void SystemInterface::resize(int new_cols, int new_rows)
{
    terminal.resize(new_cols, new_rows);
    dirty_lines.assign(new_rows, true);
    texture_cache.resize(new_rows);
    for (auto &spans : texture_cache) {
        for (auto &span : spans) {
            if (span.texture) {
                SDL_DestroyTexture(static_cast<SDL_Texture *>(span.texture));
                span.texture = nullptr;
            }
        }
        spans.clear();
    }

    struct winsize ws = {};
    ws.ws_col         = new_cols;
    ws.ws_row         = new_rows;
    if (ioctl(pty_fd, TIOCSWINSZ, &ws) < 0) {
        std::cerr << "ioctl TIOCSWINSZ failed: " << strerror(errno) << std::endl;
    }
}

int SystemInterface::get_cols() const
{
    return terminal.get_cols();
}

int SystemInterface::get_rows() const
{
    return terminal.get_rows();
}

void SystemInterface::update_texture_cache()
{
    const auto &text_buffer = terminal.get_text_buffer();
    for (size_t row = 0; row < text_buffer.size(); ++row) {
        if (!dirty_lines[row])
            continue;
        dirty_lines[row] = false;

        // Clear existing textures
        for (auto &span : texture_cache[row]) {
            if (span.texture) {
                SDL_DestroyTexture(static_cast<SDL_Texture *>(span.texture));
                span.texture = nullptr;
            }
        }
        texture_cache[row].clear();

        // Group characters into spans with the same attributes
        std::wstring current_text;
        CharAttr current_attr = text_buffer[row][0].attr;
        int start_col         = 0;

        for (int col = 0; col < terminal.get_cols(); ++col) {
            const Char &ch = text_buffer[row][col];
            if (ch.attr == current_attr && col < terminal.get_cols() - 1) {
                current_text += ch.ch;
            } else {
                if (!current_text.empty()) {
                    texture_cache[row].push_back({ current_text, current_attr, start_col });
                }
                current_text = ch.ch;
                current_attr = ch.attr;
                start_col    = col;
            }
        }
        if (!current_text.empty()) {
            texture_cache[row].push_back({ current_text, current_attr, start_col });
        }
    }
}

void SystemInterface::render_text(int row, const std::vector<TextSpan> &spans)
{
    SDL_Renderer *sdl_renderer = static_cast<SDL_Renderer *>(renderer);
    TTF_Font *ttf_font         = static_cast<TTF_Font *>(font);
    int char_width, char_height;
    TTF_SizeText(ttf_font, "A", &char_width, &char_height);

    for (const auto &span : spans) {
        if (span.text.empty())
            continue;

        // Render background
        SDL_Rect bg_rect = { span.start_col * char_width, row * char_height,
                             static_cast<int>(span.text.length()) * char_width, char_height };
        SDL_SetRenderDrawColor(sdl_renderer, span.attr.bg_r, span.attr.bg_g, span.attr.bg_b,
                               span.attr.bg_a);
        SDL_RenderFillRect(sdl_renderer, &bg_rect);

        // Create texture if not cached
        if (!span.texture) {
            SDL_Color fg_color = { span.attr.fg_r, span.attr.fg_g, span.attr.fg_b, span.attr.fg_a };
            SDL_Surface *surface = TTF_RenderUNICODE_Blended(
                static_cast<TTF_Font *>(font), reinterpret_cast<const Uint16 *>(span.text.c_str()),
                fg_color);
            if (!surface) {
                std::cerr << "TTF_RenderUNICODE_Blended failed: " << TTF_GetError() << std::endl;
                continue;
            }
            SDL_Texture *texture = SDL_CreateTextureFromSurface(sdl_renderer, surface);
            SDL_FreeSurface(surface);
            if (!texture) {
                std::cerr << "SDL_CreateTextureFromSurface failed: " << SDL_GetError() << std::endl;
                continue;
            }
            const_cast<TextSpan &>(span).texture = texture;
        }

        // Render texture
        SDL_Rect dst_rect = { span.start_col * char_width, row * char_height, 0, 0 };
        SDL_QueryTexture(static_cast<SDL_Texture *>(span.texture), nullptr, nullptr, &dst_rect.w,
                         &dst_rect.h);
        SDL_RenderCopy(sdl_renderer, static_cast<SDL_Texture *>(span.texture), nullptr, &dst_rect);
    }
}

void SystemInterface::clear_texture_cache()
{
    for (auto &row : texture_cache) {
        for (auto &span : row) {
            if (span.texture) {
                SDL_DestroyTexture(static_cast<SDL_Texture *>(span.texture));
                span.texture = nullptr;
            }
        }
        row.clear();
    }
    dirty_lines.assign(dirty_lines.size(), true);
}
