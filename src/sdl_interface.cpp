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
#include "sdl_interface.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <iostream>
#include <codecvt>

// Static signal handler context
static SdlInterface *interface_instance = nullptr;

SdlInterface::SdlInterface(int cols, int rows)
    : term_cols(cols), term_rows(rows), terminal_logic(cols, rows)
{
    interface_instance = this;
#ifdef __APPLE__
    font_path = "/System/Library/Fonts/Menlo.ttc";
#else
    font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
#endif
}

SdlInterface::~SdlInterface()
{
    if (child_pid > 0) {
        kill(child_pid, SIGTERM);
        int status;
        waitpid(child_pid, &status, 0);
    }
    if (master_fd != -1)
        close(master_fd);
    for (auto &line_spans : texture_cache) {
        for (auto &span : line_spans) {
            if (span.texture)
                SDL_DestroyTexture(static_cast<SDL_Texture *>(span.texture));
        }
    }
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    if (font)
        TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
}

bool SdlInterface::initialize()
{
    if (!initialize_sdl())
        return false;

    struct termios slave_termios;
    char *slave_name;
    if (!initialize_pty(slave_termios, slave_name))
        return false;
    if (!initialize_child_process(slave_name, slave_termios))
        return false;

    texture_cache.resize(term_rows);
    dirty_lines.resize(term_rows, true);

    return true;
}

bool SdlInterface::initialize_sdl()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    if (TTF_Init() < 0) {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        return false;
    }

    font = TTF_OpenFont(font_path.c_str(), font_size);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        return false;
    }

    TTF_SizeText(font, "M", &char_width, &char_height);
    if (char_width == 0 || char_height == 0) {
        std::cerr << "Failed to get font metrics" << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Terminal Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              term_cols * char_width, term_rows * char_height,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Cannot access GUI display.\n";
        std::cerr << "Please ensure a graphical environment is available.\n";
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

bool SdlInterface::initialize_pty(struct termios &slave_termios, char *&slave_name)
{
    master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd == -1) {
        std::cerr << "Error opening pseudo-terminal: " << strerror(errno) << std::endl;
        return false;
    }

    if (grantpt(master_fd) == -1 || unlockpt(master_fd) == -1) {
        std::cerr << "Warning: PTY setup failed: " << strerror(errno) << std::endl;
    }

    slave_name = ptsname(master_fd);
    if (!slave_name) {
        std::cerr << "Error getting slave name: " << strerror(errno) << std::endl;
        close(master_fd);
        return false;
    }

    tcgetattr(STDIN_FILENO, &slave_termios);
    slave_termios.c_lflag |= ISIG;
    slave_termios.c_iflag |= ICRNL;
    slave_termios.c_oflag |= OPOST | ONLCR;

    fcntl(master_fd, F_SETFL, O_NONBLOCK);
    return true;
}

bool SdlInterface::initialize_child_process(const char *slave_name,
                                               const struct termios &slave_termios)
{
    child_pid = fork();
    if (child_pid == -1) {
        std::cerr << "Error forking: " << strerror(errno) << std::endl;
        close(master_fd);
        return false;
    }

    if (child_pid == 0) {
        close(master_fd);
        if (setsid() == -1) {
            std::cerr << "Error setting session: " << strerror(errno) << std::endl;
            _exit(1);
        }

        int slave_fd = open(slave_name, O_RDWR);
        if (slave_fd == -1) {
            std::cerr << "Error opening slave: " << strerror(errno) << std::endl;
            _exit(1);
        }

        if (ioctl(slave_fd, TIOCSCTTY, 0) == -1 ||
            tcsetattr(slave_fd, TCSANOW, &slave_termios) == -1) {
            std::cerr << "Error setting slave terminal: " << strerror(errno) << std::endl;
            _exit(1);
        }

        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);
        if (slave_fd > 2)
            close(slave_fd);

        execl("/bin/sh", "sh", nullptr);
        std::cerr << "Error executing shell: " << strerror(errno) << std::endl;
        _exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = forward_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGQUIT, &sa, nullptr);

    sa.sa_handler = handle_sigwinch;
    sigaction(SIGWINCH, &sa, nullptr);

    return true;
}

void SdlInterface::run()
{
    bool running = true;
    while (running) {
        handle_events();
        process_pty_input();
        render_text();

        int status;
        if (waitpid(child_pid, &status, WNOHANG) > 0) {
            running = false;
        }
    }
}

void SdlInterface::render_text()
{
    Uint32 current_time = SDL_GetTicks();
    if (current_time - last_cursor_toggle >= cursor_blink_interval) {
        cursor_visible     = !cursor_visible;
        last_cursor_toggle = current_time;
    }

    update_texture_cache();
    render_spans();
    render_cursor();

    SDL_RenderPresent(renderer);
}

static std::string wstring_to_utf8(const std::wstring &wstr)
{
    std::string utf8;
    for (wchar_t wc : wstr) {
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
    }
    return utf8;
}

void SdlInterface::update_texture_cache()
{
    const auto &text_buffer = terminal_logic.get_text_buffer();

    for (size_t i = 0; i < text_buffer.size(); ++i) {
        if (!dirty_lines[i])
            continue;

        for (auto &span : texture_cache[i]) {
            if (span.texture)
                SDL_DestroyTexture(static_cast<SDL_Texture *>(span.texture));
        }
        texture_cache[i].clear();

        std::wstring current_text;
        CharAttr current_span_attr = text_buffer[i][0].attr;
        int start_col              = 0;

        for (int j = 0; j < term_cols; ++j) {
            const auto &c = text_buffer[i][j];
            if (c.attr == current_span_attr && j < term_cols - 1) {
                current_text += c.ch;
            } else {
                if (!current_text.empty()) {
                    TextSpan span;
                    span.text      = current_text;
                    span.attr      = current_span_attr;
                    span.start_col = start_col;

                    SDL_Color fg         = { current_span_attr.fg_r, current_span_attr.fg_g,
                                             current_span_attr.fg_b, current_span_attr.fg_a };
                    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, wstring_to_utf8(current_text).c_str(), fg);
                    if (surface) {
                        span.texture = SDL_CreateTextureFromSurface(renderer, surface);
                        SDL_FreeSurface(surface);
                    }
                    texture_cache[i].push_back(span);
                }
                current_text      = c.ch;
                current_span_attr = c.attr;
                start_col         = j;
            }
        }

        if (!current_text.empty()) {
            TextSpan span;
            span.text      = current_text;
            span.attr      = current_span_attr;
            span.start_col = start_col;

            SDL_Color fg = { current_span_attr.fg_r, current_span_attr.fg_g, current_span_attr.fg_b,
                             current_span_attr.fg_a };
            SDL_Surface *surface = TTF_RenderUTF8_Blended(font, wstring_to_utf8(current_text).c_str(), fg);
            if (surface) {
                span.texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_FreeSurface(surface);
            }
            texture_cache[i].push_back(span);
        }

        dirty_lines[i] = false;
    }
}

void SdlInterface::render_spans()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (size_t i = 0; i < texture_cache.size() && i < static_cast<size_t>(term_rows); ++i) {
        for (const auto &span : texture_cache[i]) {
            if (!span.texture)
                continue;

            SDL_SetRenderDrawColor(renderer, span.attr.bg_r, span.attr.bg_g, span.attr.bg_b,
                                   span.attr.bg_a);
            SDL_Rect bg_rect = { span.start_col * char_width, static_cast<int>(i * char_height),
                                 static_cast<int>(span.text.length() * char_width), char_height };
            SDL_RenderFillRect(renderer, &bg_rect);

            int w, h;
            SDL_QueryTexture(static_cast<SDL_Texture *>(span.texture), nullptr, nullptr, &w, &h);
            SDL_Rect dst = { span.start_col * char_width, static_cast<int>(i * char_height), w, h };
            SDL_RenderCopy(renderer, static_cast<SDL_Texture *>(span.texture), nullptr, &dst);
        }
    }
}

void SdlInterface::render_cursor()
{
    if (cursor_visible) {
        const auto &cursor = terminal_logic.get_cursor();
        if (cursor.row < term_rows && cursor.col < term_cols) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_Rect cursor_rect = { cursor.col * char_width, cursor.row * char_height, char_width,
                                     char_height };
            SDL_RenderFillRect(renderer, &cursor_rect);
        }
    }
}

void SdlInterface::handle_events()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            kill(child_pid, SIGTERM);
            break;
        case SDL_KEYDOWN:
            handle_key_event(event.key);
            break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                int new_cols = std::max(event.window.data1 / char_width, 1);
                int new_rows = std::max(event.window.data2 / char_height, 1);
                terminal_logic.resize(new_cols, new_rows);
                texture_cache.resize(new_rows);
                dirty_lines.resize(new_rows, true);
                term_cols = new_cols;
                term_rows = new_rows;

                struct winsize ws;
                ws.ws_col    = term_cols;
                ws.ws_row    = term_rows;
                ws.ws_xpixel = term_cols * char_width;
                ws.ws_ypixel = term_rows * char_height;
                if (ioctl(master_fd, TIOCSWINSZ, &ws) == -1) {
                    std::cerr << "Error setting slave window size: " << strerror(errno)
                              << std::endl;
                }
                if (child_pid > 0) {
                    kill(child_pid, SIGWINCH);
                }
            }
            break;
        }
    }
}

void SdlInterface::handle_key_event(const SDL_KeyboardEvent &key)
{
    // Handle font size changes
#ifdef __APPLE__
    if (key.keysym.mod & KMOD_GUI) {
        if (key.keysym.sym == SDLK_EQUALS) {
            change_font_size(1); // Cmd+=
            return;
        } else if (key.keysym.sym == SDLK_MINUS) {
            change_font_size(-1); // Cmd+-
            return;
        }
    }
#else
    if (key.keysym.mod & KMOD_CTRL) {
        if (key.keysym.sym == SDLK_EQUALS) {
            change_font_size(1); // Ctrl+=
            return;
        } else if (key.keysym.sym == SDLK_MINUS) {
            change_font_size(-1); // Ctrl+-
            return;
        }
    }
#endif

    // Forward key to TerminalLogic
    std::string input = terminal_logic.process_key(keysym_to_key_input(key.keysym));
    if (!input.empty()) {
        // std::cerr << "Sending input: ";
        // for (char c : input) {
        //     std::cerr << (int)c << " ";
        // }
        // std::cerr << std::endl;
        write(master_fd, input.c_str(), input.size());
    }
}

KeyInput SdlInterface::keysym_to_key_input(const SDL_Keysym &keysym)
{
    KeyInput key;

    // Map SDL2 keycodes to KeyCode enum
    switch (keysym.sym) {
    case SDLK_RETURN:
        key.code = KeyCode::ENTER;
        break;
    case SDLK_BACKSPACE:
        key.code = KeyCode::BACKSPACE;
        break;
    case SDLK_TAB:
        key.code = KeyCode::TAB;
        break;
    case SDLK_ESCAPE:
        key.code = KeyCode::ESCAPE;
        break;
    case SDLK_UP:
        key.code = KeyCode::UP;
        break;
    case SDLK_DOWN:
        key.code = KeyCode::DOWN;
        break;
    case SDLK_RIGHT:
        key.code = KeyCode::RIGHT;
        break;
    case SDLK_LEFT:
        key.code = KeyCode::LEFT;
        break;
    case SDLK_HOME:
        key.code = KeyCode::HOME;
        break;
    case SDLK_END:
        key.code = KeyCode::END;
        break;
    case SDLK_INSERT:
        key.code = KeyCode::INSERT;
        break;
    case SDLK_DELETE:
        key.code = KeyCode::DELETE;
        break;
    case SDLK_PAGEUP:
        key.code = KeyCode::PAGEUP;
        break;
    case SDLK_PAGEDOWN:
        key.code = KeyCode::PAGEDOWN;
        break;
    case SDLK_F1:
        key.code = KeyCode::F1;
        break;
    case SDLK_F2:
        key.code = KeyCode::F2;
        break;
    case SDLK_F3:
        key.code = KeyCode::F3;
        break;
    case SDLK_F4:
        key.code = KeyCode::F4;
        break;
    case SDLK_F5:
        key.code = KeyCode::F5;
        break;
    case SDLK_F6:
        key.code = KeyCode::F6;
        break;
    case SDLK_F7:
        key.code = KeyCode::F7;
        break;
    case SDLK_F8:
        key.code = KeyCode::F8;
        break;
    case SDLK_F9:
        key.code = KeyCode::F9;
        break;
    case SDLK_F10:
        key.code = KeyCode::F10;
        break;
    case SDLK_F11:
        key.code = KeyCode::F11;
        break;
    case SDLK_F12:
        key.code = KeyCode::F12;
        break;
    default:
        key.code = KeyCode::CHARACTER;
        key.character = static_cast<wchar_t>(keysym.sym);
        break;
    }

    key.mod_shift = keysym.mod & KMOD_SHIFT;
    key.mod_ctrl  = keysym.mod & KMOD_CTRL;
    return key;
}

void SdlInterface::change_font_size(int delta)
{
    int new_size = font_size + delta;
    if (new_size < 8 || new_size > 72) {
        // std::cerr << "Font size out of range: " << new_size << std::endl;
        return;
    }

    if (font) {
        TTF_CloseFont(font);
        font = nullptr;
    }

    font_size = new_size;
    font      = TTF_OpenFont(font_path.c_str(), font_size);
    if (!font) {
        std::cerr << "Failed to load font at size " << font_size << ": " << TTF_GetError()
                  << std::endl;
        font_size = 16;
        font      = TTF_OpenFont(font_path.c_str(), font_size);
        if (!font) {
            std::cerr << "Failed to revert to default font: " << TTF_GetError() << std::endl;
            return;
        }
    }

    TTF_SizeText(font, "M", &char_width, &char_height);
    if (char_width == 0 || char_height == 0) {
        std::cerr << "Failed to get font metrics for size " << font_size << std::endl;
        TTF_CloseFont(font);
        font = nullptr;
        return;
    }

    SDL_SetWindowSize(window, term_cols * char_width, term_rows * char_height);

    int win_width, win_height;
    SDL_GetWindowSize(window, &win_width, &win_height);
    int new_cols = std::max(win_width / char_width, 1);
    int new_rows = std::max(win_height / char_height, 1);
    term_cols    = new_cols;
    term_rows    = new_rows;

    struct winsize ws;
    ws.ws_col    = term_cols;
    ws.ws_row    = term_rows;
    ws.ws_xpixel = term_cols * char_width;
    ws.ws_ypixel = term_rows * char_height;
    if (ioctl(master_fd, TIOCSWINSZ, &ws) == -1) {
        std::cerr << "Error setting slave window size: " << strerror(errno) << std::endl;
    } else {
        // std::cerr << "Updated slave size to " << ws.ws_col << "x" << ws.ws_row << std::endl;
    }

    if (child_pid > 0) {
        kill(child_pid, SIGWINCH);
    }

    terminal_logic.resize(term_cols, term_rows);
    for (auto &line_spans : texture_cache) {
        for (auto &span : line_spans) {
            if (span.texture)
                SDL_DestroyTexture(static_cast<SDL_Texture *>(span.texture));
        }
    }
    texture_cache.resize(term_rows);
    dirty_lines.resize(term_rows, true);

    // std::cerr << "Changed font size to " << font_size << ", terminal size to " << term_cols <<
    // "x"
    //           << term_rows << std::endl;
}

void SdlInterface::process_pty_input()
{
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(master_fd, &read_fds);
    struct timeval tv = { 0, 10000 };

    if (select(master_fd + 1, &read_fds, nullptr, nullptr, &tv) <= 0)
        return;

    if (FD_ISSET(master_fd, &read_fds)) {
        char buffer[1024];
        ssize_t bytes = read(master_fd, buffer, sizeof(buffer) - 1);
        if (bytes <= 0) {
            if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cerr << "Error reading from master_fd: " << strerror(errno) << std::endl;
                kill(child_pid, SIGTERM);
            }
            return;
        }

        buffer[bytes] = '\0';
        // std::cerr << "Read " << bytes << " bytes: ";
        // for (ssize_t j = 0; j < bytes; ++j) {
        //     std::cerr << (int)buffer[j] << " ";
        // }
        // std::cerr << std::endl;

        // Process input through TerminalLogic
        auto dirty_rows = terminal_logic.process_input(buffer, bytes);
        for (int row : dirty_rows) {
            if (row >= 0 && static_cast<size_t>(row) < dirty_lines.size()) {
                dirty_lines[row] = true;
            }
        }
    }
}

void SdlInterface::forward_signal(int sig)
{
    if (interface_instance && interface_instance->child_pid > 0) {
        kill(interface_instance->child_pid, sig);
    }
}

void SdlInterface::handle_sigwinch(int sig)
{
    if (interface_instance) {
        int win_width, win_height;
        SDL_GetWindowSize(interface_instance->window, &win_width, &win_height);
        int new_cols = std::max(win_width / interface_instance->char_width, 1);
        int new_rows = std::max(win_height / interface_instance->char_height, 1);

        interface_instance->term_cols = new_cols;
        interface_instance->term_rows = new_rows;

        struct winsize ws;
        ws.ws_col    = new_cols;
        ws.ws_row    = new_rows;
        ws.ws_xpixel = new_cols * interface_instance->char_width;
        ws.ws_ypixel = new_rows * interface_instance->char_height;
        if (ioctl(interface_instance->master_fd, TIOCSWINSZ, &ws) == -1) {
            std::cerr << "Error setting slave window size: " << strerror(errno) << std::endl;
            return;
        }

        interface_instance->terminal_logic.resize(new_cols, new_rows);
        for (auto &line_spans : interface_instance->texture_cache) {
            for (auto &span : line_spans) {
                if (span.texture)
                    SDL_DestroyTexture(static_cast<SDL_Texture *>(span.texture));
            }
        }
        interface_instance->texture_cache.resize(new_rows);
        interface_instance->dirty_lines.resize(new_rows, true);

        if (interface_instance->child_pid > 0) {
            kill(interface_instance->child_pid, SIGWINCH);
        }
    }
}
