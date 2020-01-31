#include "game.hpp"
#include <conio.h>
#include <iostream>
#include <string.h>
#include <math.h>

#ifndef _WIN32_WINNT 
#define _WIN32_WINNT 0x0500
#endif

#define PANEL_WIDTH 19
#define PENALTY 400
#define BONUS 200


//TODO: frame-independent input (change sleep timer here as well probably)
//      allow window resizing
//      clear side panel completely
//      check if all drawing is done properly
//      add game objects (shield, maybe more ambitious shit)
//      maybe change how enemy car works

Game::Game() {
    state_ = Game_state::running;
    current_map_ = new Map(1);
    map_list_ = new maps_ll;
    map_list_->map = current_map_;
    map_list_->next = NULL;
    score_ = 1;
    level_ = 1;

    input_buffer_ = GetStdHandle(STD_INPUT_HANDLE);

    active_screen_buffer_ = GetStdHandle(STD_OUTPUT_HANDLE);
    _CONSOLE_CURSOR_INFO cursor_info = {1, false};
    SetConsoleCursorInfo(active_screen_buffer_, &cursor_info);
    _SMALL_RECT wSize = {0, 0, MAP_WIDTH + PANEL_WIDTH - 1, MAP_HEIGHT - 1};
    SetConsoleWindowInfo(active_screen_buffer_, TRUE, &wSize);
    _COORD buffer_size = {MAP_WIDTH + PANEL_WIDTH, MAP_HEIGHT};
    SetConsoleScreenBufferSize(active_screen_buffer_, buffer_size);
    CONSOLE_FONT_INFOEX font_info = {sizeof(CONSOLE_FONT_INFOEX)};
    GetCurrentConsoleFontEx(active_screen_buffer_, FALSE, &font_info);
    font_info.dwFontSize.X = 14;
    font_info.dwFontSize.Y = 24;
    SetCurrentConsoleFontEx(active_screen_buffer_, FALSE,  &font_info);

    secondary_screen_buffer_ = CreateConsoleScreenBuffer(GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleCursorInfo(secondary_screen_buffer_, &cursor_info);
}

void Game::play() {
    LARGE_INTEGER frequency, tick_count, prev_tick_count;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&tick_count);
    int dt = 0;

    while (state_ != Game_state::over) {
        char input = get_input();

        if (input == 'p') {
            state_ = Game_state::paused;
            FlushConsoleInputBuffer(input_buffer_);
        }

        if (state_ == Game_state::running) {
            prev_tick_count = tick_count;
            QueryPerformanceCounter(&tick_count);
            dt += (tick_count.QuadPart - prev_tick_count.QuadPart) * 1000 / frequency.QuadPart;

            if (dt > 200 * pow(0.90, level_)) {
                score_ += 10;
                current_map_->advance();
                dt = 0;
            }

            bool wall_hit = current_map_->player_move_sideways(input);
            if (wall_hit) {
                score_ -= PENALTY;
            }

            char collision = current_map_->player_collision();
            while (collision) {
                switch (collision) {
                    case 'X':
                        score_ -= PENALTY;
                        break;
                    case 'B':
                        score_ += BONUS;
                        break;
                    case 'E':
                        score_ -= 2 * PENALTY;
                        break;
                }
                collision = current_map_->player_collision();
            }

            current_map_->draw(secondary_screen_buffer_);
            draw_panel(secondary_screen_buffer_);
            swap_buffers();
            Sleep(10);
            update_game_state();
            
        }

        else if (state_ == Game_state::paused) {
            draw_panel(secondary_screen_buffer_);
            swap_buffers();
            char input = _getch();
            if (input == 'q') {
                state_ = Game_state::over;
            }
            else {
                state_ = Game_state::running;
            }
        }

    }
}

char Game::get_input() {
    char c = 0;
    if (_kbhit()) {
        c = _getch();
    }
    return c;
}

void Game::draw_panel(HANDLE screen_buffer) {
    DWORD test = 0;

    if (state_ == Game_state::paused) {
        _COORD pause_coord1 = {MAP_WIDTH + 2, 11};
        char pause_string1[PANEL_WIDTH] = "GAME IS PAUSED";
        WriteConsoleOutputCharacterA(screen_buffer, pause_string1, strlen(pause_string1), pause_coord1, &test);

        _COORD pause_coord2 = {MAP_WIDTH + 2, 13};
        char pause_string2[PANEL_WIDTH] = "PRESS ANY KEY";
        WriteConsoleOutputCharacterA(screen_buffer, pause_string2, strlen(pause_string2), pause_coord2, &test);

        _COORD pause_coord3 = {MAP_WIDTH + 3, 14};
        char pause_string3[PANEL_WIDTH] = "TO CONTINUE";
        WriteConsoleOutputCharacterA(screen_buffer, pause_string3, strlen(pause_string3), pause_coord3, &test);

        _COORD pause_coord4 = {MAP_WIDTH + 4, 16};
        char pause_string4[PANEL_WIDTH] = "Q TO QUIT";
        WriteConsoleOutputCharacterA(screen_buffer, pause_string4, strlen(pause_string4), pause_coord4, &test);
    }

    else if (state_ == Game_state::running) {
        for (char i = 0; i < MAP_HEIGHT; i++) {
            _COORD panel_border = {MAP_WIDTH + PANEL_WIDTH - 1, i};
            WriteConsoleOutputCharacterA(screen_buffer, "#", 1, panel_border, &test);
        }

        _COORD clear_panel1 = {MAP_WIDTH + 2, 11};
        FillConsoleOutputCharacterA(screen_buffer, ' ', 16, clear_panel1, &test);

        _COORD clear_panel2 = {MAP_WIDTH + 2, 13};
        FillConsoleOutputCharacterA(screen_buffer, ' ', 16, clear_panel2, &test);

        _COORD clear_panel3 = {MAP_WIDTH + 3, 14};
        FillConsoleOutputCharacterA(screen_buffer, ' ', 15, clear_panel3, &test);

        _COORD clear_panel4 = {MAP_WIDTH + 4, 16};
        FillConsoleOutputCharacterA(screen_buffer, ' ', 14, clear_panel4, &test);

        _COORD score_clear = {MAP_WIDTH + 9, 3};
        FillConsoleOutputCharacterA(screen_buffer, ' ', 8, score_clear, &test);

        _COORD score_coord = {MAP_WIDTH + 2, 3};
        char score_string[PANEL_WIDTH] = "SCORE: ";
        char score[10];
        itoa(score_, score, 10);
        strcat(score_string, score);
        WriteConsoleOutputCharacterA(screen_buffer, score_string, strlen(score_string), score_coord, &test);

        _COORD level_clear = {MAP_WIDTH + 9, 5};
        FillConsoleOutputCharacterA(screen_buffer, ' ', 8, level_clear, &test);

        _COORD level_coord = {MAP_WIDTH + 2, 5};
        char level_string[PANEL_WIDTH] = "LEVEL: ";
        char level[3];
        itoa(level_, level, 10);
        strcat(level_string, level);
        WriteConsoleOutputCharacterA(screen_buffer, level_string, strlen(level_string), level_coord, &test);
    }
}

Map* Game::get_map(int level) {
    maps_ll* iter = map_list_;
    for (int i = 1; i < level; i++) {
        iter = iter->next;
        if (!iter) {
            return NULL;
        }
    }

    return iter->map;
}

void Game::append_map_list(Map* new_map) {
    maps_ll* iter = map_list_;
    for (int i = 1; i < level_ - 1; i++) {
        iter = iter->next;
    }
    iter->next = new maps_ll;
    iter = iter->next;
    iter->map = new_map;
    iter->next = NULL;
}

void Game::swap_buffers() {
    HANDLE temp = secondary_screen_buffer_;
    secondary_screen_buffer_ = active_screen_buffer_;
    active_screen_buffer_ = temp;
    SetConsoleActiveScreenBuffer(active_screen_buffer_);
}

void Game::update_game_state() {
    if (score_ < 0) {
        state_ = Game_state::over;
    }

    else if (score_ <= (level_ - 1) * 2000) {
        level_ -= 1;
        score_ -= 800;
        current_map_ = get_map(level_);
    }

    else if (score_ >= level_ * 2000) {
        level_ += 1;
        current_map_ = get_map(level_);
        if (!current_map_) {
            current_map_ = new Map(level_);
            append_map_list(current_map_);
        }
    }
}