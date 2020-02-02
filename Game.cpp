#include "game.hpp"
#include <conio.h>
#include <iostream>
#include <string.h>
#include <math.h>

#ifndef _WIN32_WINNT 
#define _WIN32_WINNT 0x0500
#endif

#define PENALTY 400
#define BONUS 200


//TODO: starting and victory screen

Game::Game() {
    state_ = Game_state::running;
    current_map_ = new Map(1);
    map_list_ = new maps_ll;
    map_list_->map = current_map_;
    map_list_->next = NULL;
    score_ = 0;
    level_ = 1;

    active_screen_buffer_ = GetStdHandle(STD_OUTPUT_HANDLE);
    _CONSOLE_CURSOR_INFO cursor_info = {1, false};
    SetConsoleCursorInfo(active_screen_buffer_, &cursor_info);
    _SMALL_RECT wSize = {0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1};
    SetConsoleWindowInfo(active_screen_buffer_, TRUE, &wSize);
    _COORD buffer_size = {SCREEN_WIDTH, SCREEN_HEIGHT};
    SetConsoleScreenBufferSize(active_screen_buffer_, buffer_size);
    CONSOLE_FONT_INFOEX font_info = {sizeof(CONSOLE_FONT_INFOEX)};
    GetCurrentConsoleFontEx(active_screen_buffer_, FALSE, &font_info);
    font_info.dwFontSize.Y = 28;
    SetCurrentConsoleFontEx(active_screen_buffer_, FALSE,  &font_info);

    secondary_screen_buffer_ = CreateConsoleScreenBuffer(GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleCursorInfo(secondary_screen_buffer_, &cursor_info);
    SetCurrentConsoleFontEx(secondary_screen_buffer_, FALSE,  &font_info);
}


void Game::play() {
    LARGE_INTEGER frequency, tick_count, prev_tick_count;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&tick_count);
    double dt = 0;
    double fps;

    while (state_ != Game_state::over) {
        char input = get_input();
        if (input == 'p') {
            state_ = Game_state::paused;
        }

        prev_tick_count = tick_count;
        QueryPerformanceCounter(&tick_count);
        dt += (double) (tick_count.QuadPart - prev_tick_count.QuadPart) * 1000 / frequency.QuadPart;
        fps = (double) frequency.QuadPart / (tick_count.QuadPart - prev_tick_count.QuadPart);

        if (state_ == Game_state::running) {

            if (dt > 200 * pow(0.94, level_)) {
                score_ += 10;
                current_map_->advance();
                dt = 0;
            }

            bool wall_hit = current_map_->player_move_sideways(input);
            calculate_collisions(wall_hit);

            clear_screen_grid();
            current_map_->draw(screen_grid_);
            draw_panel(fps);
            
            clear_screen_buffer();
            render_screen_grid();

            update_game_state();
            //Sleep(2);
        }

        else if (state_ == Game_state::paused) {
            clear_screen_grid();
            current_map_->draw(screen_grid_);
            draw_panel(fps);
            
            clear_screen_buffer();
            render_screen_grid();
            
            char input = _getch();
            if (input == 'q') {
                state_ = Game_state::over;
            }
            else {
                QueryPerformanceCounter(&tick_count);
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


void Game::calculate_collisions(int wall_hit) {
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
}


//DEBUG MODE
void Game::draw_panel(double fps) {

    for (char i = 0; i < MAP_HEIGHT; i++) {
        screen_grid_[SCREEN_WIDTH * (i + 1) - 1].Char.AsciiChar = '#';
        screen_grid_[SCREEN_WIDTH * (i + 1) - 1].Attributes = FOREGROUND_WHITE;
    }

    char score_string[PANEL_WIDTH] = "SCORE: ";
    char score[12];
    _itoa(score_, score, 10);
    strcat(score_string, score);
    draw_string(score_string, FOREGROUND_WHITE, MAP_WIDTH + 2, 2);

    char level_string[PANEL_WIDTH] = "LEVEL: ";
    char level[12];
    _itoa(level_, level, 10);
    strcat(level_string, level);
    draw_string(level_string, FOREGROUND_WHITE, MAP_WIDTH + 2, 4);

    char fps_string[PANEL_WIDTH] = "fps: ";
    char fps_s[12];
    _itoa(fps, fps_s, 10);
    strcat(fps_string, fps_s);
    draw_string(fps_string, FOREGROUND_WHITE, MAP_WIDTH + 2, 6);

    if (state_ == Game_state::paused) {
        char pause_string1[PANEL_WIDTH] = "GAME IS PAUSED";
        draw_string(pause_string1, FOREGROUND_WHITE, MAP_WIDTH + 2, 10);

        char pause_string2[PANEL_WIDTH] = "PRESS ANY KEY";
        draw_string(pause_string2, FOREGROUND_WHITE, MAP_WIDTH + 2, 12);

        char pause_string3[PANEL_WIDTH] = "TO CONTINUE";
        draw_string(pause_string3, FOREGROUND_WHITE, MAP_WIDTH + 3, 13);

        char pause_string4[PANEL_WIDTH] = "Q TO QUIT";
        draw_string(pause_string4, FOREGROUND_WHITE, MAP_WIDTH + 4, 15);   
    }
}


void Game::update_game_state() {
    if (score_ < 0) {
        state_ = Game_state::over;
    }

    else if (score_ < (level_ - 1) * 2000) {
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


Map* Game::get_map(int level) {
    maps_ll* iter = map_list_;
    for (int i = 1; i < level; i++) {
        iter = iter->next;
    }

    if (!iter) {
        return NULL;
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

    
void Game::clear_screen_grid() {
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < SCREEN_WIDTH; j++) {
            screen_grid_[j + i * SCREEN_WIDTH].Char.AsciiChar = ' ';
            screen_grid_[j + i * SCREEN_WIDTH].Attributes = 0;
        }
    }
}

void Game::draw_string(char* string, short attributes, int x_pos, int y_pos ) {
    while (*string) {
        screen_grid_[x_pos + y_pos * SCREEN_WIDTH].Char.AsciiChar = *string;
        screen_grid_[x_pos + y_pos * SCREEN_WIDTH].Attributes = attributes;
        string++;
        x_pos++;
    }
}

void Game::clear_screen_buffer() {
    DWORD temp;
    _COORD origin = {0, 0};
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    GetConsoleScreenBufferInfo(secondary_screen_buffer_, &buffer_info);
    FillConsoleOutputCharacterA(secondary_screen_buffer_, ' ', buffer_info.dwSize.X * buffer_info.dwSize.Y, origin, &temp); 
}

void Game::render_screen_grid() {
    _COORD origin = {0, 0};
    _COORD screen_size = {SCREEN_WIDTH, SCREEN_HEIGHT};
    SMALL_RECT write_region = {0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1};
    WriteConsoleOutputA(secondary_screen_buffer_, screen_grid_, screen_size, origin, &write_region);
    swap_buffers();
}

void Game::swap_buffers() {
    HANDLE temp = secondary_screen_buffer_;
    secondary_screen_buffer_ = active_screen_buffer_;
    active_screen_buffer_ = temp;
    SetConsoleActiveScreenBuffer(active_screen_buffer_);
}


void Game::start_screen() {
    

}


void Game::end_screen() {

    
}