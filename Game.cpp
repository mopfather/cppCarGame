#include "game.hpp"
#include <conio.h>
#include <iostream>
#include <string.h>
#include <math.h>
#include <stdio.h>

#ifndef _WIN32_WINNT 
#define _WIN32_WINNT 0x0500
#endif

#define PENALTY 400
#define BONUS 200


Game::Game() {
    state_ = Game_state::running;
    current_map_ = new Map(1);
    map_list_ = new maps_ll;
    map_list_->map = current_map_;
    map_list_->next = NULL;
    score_ = 0;
    level_ = 1;
    
    _CONSOLE_CURSOR_INFO cursor_info = {1, FALSE};
    _SMALL_RECT wSize = {0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1};
    _COORD buffer_size = {SCREEN_WIDTH, SCREEN_HEIGHT};
    CONSOLE_FONT_INFOEX font_info = {sizeof(CONSOLE_FONT_INFOEX)};

    input_buffer_ = GetStdHandle(STD_INPUT_HANDLE);

    active_screen_buffer_ = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorInfo(active_screen_buffer_, &cursor_info);
    SetConsoleWindowInfo(active_screen_buffer_, TRUE, &wSize);
    SetConsoleScreenBufferSize(active_screen_buffer_, buffer_size);
    GetCurrentConsoleFontEx(active_screen_buffer_, FALSE, &font_info);
    font_info.dwFontSize.Y = 24;
    SetCurrentConsoleFontEx(active_screen_buffer_, FALSE,  &font_info);

    secondary_screen_buffer_ = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
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
        prev_tick_count = tick_count;
        QueryPerformanceCounter(&tick_count);
        dt += (double) (tick_count.QuadPart - prev_tick_count.QuadPart) * 1000 / frequency.QuadPart;
        fps = (double) frequency.QuadPart / (tick_count.QuadPart - prev_tick_count.QuadPart);
        
        char input = get_input();
        if (input == 'p' && state_ == Game_state::running) {
            state_ = Game_state::paused;
        }
        else if (input == 'q' && state_ == Game_state::paused) {
            state_ = Game_state::over;
        }
        else if (input && state_ == Game_state::paused) {
            state_ = Game_state::running;
        }

        if (state_ == Game_state::running) {

            if (dt > 200 * pow(0.94, level_)) {
                score_ += 10;
                current_map_->advance();
                dt = 0;
            }

            bool wall_hit = current_map_->player_move_sideways(input);
            calculate_collisions(wall_hit);
        }
        
        clear_screen_grid();
        current_map_->draw(screen_grid_);
        draw_panel(fps);
        
        clear_screen_buffer();
        render_screen_grid();

        update_game_state();
        Sleep(1);
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

    char fps_string[PANEL_WIDTH] = "FPS: ";
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
            screen_grid_[j + i * SCREEN_WIDTH].Attributes = FOREGROUND_WHITE;
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
    DWORD test;
    _COORD origin = {0, 0};
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    GetConsoleScreenBufferInfo(secondary_screen_buffer_, &buffer_info);
    FillConsoleOutputCharacterA(secondary_screen_buffer_, ' ', buffer_info.dwSize.X * buffer_info.dwSize.Y, origin, &test);
    FillConsoleOutputAttribute(secondary_screen_buffer_, FOREGROUND_WHITE, buffer_info.dwSize.X * buffer_info.dwSize.Y, origin, &test); 
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
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    GetConsoleScreenBufferInfo(secondary_screen_buffer_, &buffer_info);

    DWORD test;
    char intro1[] = R"(                           __  __         ___ ___      ___ ___)";
    char intro2[] = R"(   ___  _______  ___ ____ / /_/ /____    |_  / _ \____|_  <  /)";
    char intro3[] = R"(  / _ \/ __/ _ \/ _ `/ -_) __/ __/ _ \  / __/ // /___/ __// / )";
    char intro4[] = R"( / .__/_/  \___/\_, /\__/\__/\__/\___/ /____|___/   /____/_/  )";
    char intro5[] = R"(/_/            /___/                                          )";
        
    char title1[] = R"( _____ _______   _______  ______  ___  _____ ___________ )";
    char title2[] = R"(|_   _|  ___\ \ / /_   _| | ___ \/ _ \/  __ \  ___| ___ \)";
    char title3[] = R"(  | | | |__  \ V /  | |   | |_/ / /_\ \ /  \/ |__ | |_/ /)";
    char title4[] = R"(  | | |  __| /   \  | |   |    /|  _  | |   |  __||    / )";
    char title5[] = R"(  | | | |___/ /^\ \ | |   | |\ \| | | | \__/\ |___| |\ \ )";
    char title6[] = R"(  \_/ \____/\/   \/ \_/   \_| \_\_| |_/\____|____/\_| \_|)";

    WriteConsoleOutputCharacterA(secondary_screen_buffer_, intro1, strlen(intro1), {2, 0}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, intro2, strlen(intro2), {2, 1}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, intro3, strlen(intro3), {2, 2}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, intro4, strlen(intro4), {2, 3}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, intro5, strlen(intro5), {2, 4}, &test);
    FillConsoleOutputAttribute(secondary_screen_buffer_, FOREGROUND_GREEN, (buffer_info.srWindow.Right + 1) * 5, {0, 0}, &test);
    FillConsoleOutputCharacterA(secondary_screen_buffer_, '=', SCREEN_WIDTH, {0, 6}, &test);

    WriteConsoleOutputCharacterA(secondary_screen_buffer_, title1, strlen(title1), {4, 7}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, title2, strlen(title2), {4, 8}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, title3, strlen(title3), {4, 9}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, title4, strlen(title4), {4, 10}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, title5, strlen(title5), {4, 11}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, title6, strlen(title6), {4, 12}, &test);
    FillConsoleOutputAttribute(secondary_screen_buffer_, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY, (buffer_info.srWindow.Right + 1) * 6, {0, 7}, &test);
    FillConsoleOutputCharacterA(secondary_screen_buffer_, '=', SCREEN_WIDTH, {0, 14}, &test);

    WriteConsoleOutputCharacterA(secondary_screen_buffer_, "PRESS ANY KEY TO CONTINUE", 25, {20, 17}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, "Giuliano Bruno", 14, {26, 21}, &test);

    swap_buffers();
    _getch();
}

void Game::end_screen() {
    _CONSOLE_CURSOR_INFO cursor_info = {1, FALSE};
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    SetConsoleCursorInfo(secondary_screen_buffer_, &cursor_info);
    GetConsoleScreenBufferInfo(secondary_screen_buffer_, &buffer_info);
    clear_screen_buffer();

    DWORD test;
    char game_over1[] = R"(   _________    __  _________   ____ _    ____________ )";
    char game_over2[] = R"(  / ____/   |  /  |/  / ____/  / __ \ |  / / ____/ __ \)";
    char game_over3[] = R"( / / __/ /| | / /|_/ / __/    / / / / | / / __/ / /_/ /)";
    char game_over4[] = R"(/ /_/ / ___ |/ /  / / /___   / /_/ /| |/ / /___/ _, _/ )";
    char game_over5[] = R"(\____/_/  |_/_/  /_/_____/   \____/ |___/_____/_/ |_|  )";

    char thanks_for1[] = R"(  ________  _____    _   ____ _______    __________  ____ )";
    char thanks_for2[] = R"( /_  __/ / / /   |  / | / / //_/ ___/   / ____/ __ \/ __ \)";
    char thanks_for3[] = R"(  / / / /_/ / /| | /  |/ / ,<  \__ \   / /_  / / / / /_/ /)";
    char thanks_for4[] = R"( / / / __  / ___ |/ /|  / /| |___/ /  / __/ / /_/ / _, _/ )";
    char thanks_for5[] = R"(/_/ /_/ /_/_/  |_/_/ |_/_/ |_/____/  /_/    \____/_/ |_|  )";

    char playing1[] = R"(    ____  __    _____  _______   ________)";
    char playing2[] = R"(   / __ \/ /   /   \ \/ /  _/ | / / ____/)";
    char playing3[] = R"(  / /_/ / /   / /| |\  // //  |/ / / __  )";
    char playing4[] = R"( / ____/ /___/ ___ |/ // // /|  / /_/ /  )";
    char playing5[] = R"(/_/   /_____/_/  |_/_/___/_/ |_/\____/   )";

    WriteConsoleOutputCharacterA(secondary_screen_buffer_, game_over1, strlen(game_over1), {5, 1}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, game_over2, strlen(game_over2), {5, 2}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, game_over3, strlen(game_over3), {5, 3}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, game_over4, strlen(game_over4), {5, 4}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, game_over5, strlen(game_over5), {5, 5}, &test);
    FillConsoleOutputAttribute(secondary_screen_buffer_, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY, (buffer_info.srWindow.Right + 1) * 5, {0, 1}, &test);
    
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, thanks_for1, strlen(thanks_for1), {3, 8}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, thanks_for2, strlen(thanks_for2), {3, 9}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, thanks_for3, strlen(thanks_for3), {3, 10}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, thanks_for4, strlen(thanks_for4), {3, 11}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, thanks_for5, strlen(thanks_for5), {3, 12}, &test);
    FillConsoleOutputAttribute(secondary_screen_buffer_, FOREGROUND_RED, (buffer_info.srWindow.Right + 1) * 5, {0, 8}, &test);

    WriteConsoleOutputCharacterA(secondary_screen_buffer_, playing1, strlen(playing1), {12, 14}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, playing2, strlen(playing2), {12, 15}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, playing3, strlen(playing3), {12, 16}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, playing4, strlen(playing4), {12, 17}, &test);
    WriteConsoleOutputCharacterA(secondary_screen_buffer_, playing5, strlen(playing5), {12, 18}, &test);
    FillConsoleOutputAttribute(secondary_screen_buffer_, FOREGROUND_RED, (buffer_info.srWindow.Right + 1) * 5, {0, 14}, &test);

    swap_buffers();
    Sleep(1000);
    FlushConsoleInputBuffer(input_buffer_);
    _getch();
}