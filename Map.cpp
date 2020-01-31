#include "Map.hpp"
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <conio.h>

enum tiles {
    tile_spikes = 'X',
    tile_bonus = 'B',
    tile_border = '#',
    tile_empty = ' ',
    tile_special = 'S'
};

Map::Map(int level) {
    player_pos_ = MAP_WIDTH/2;
    enemy_pos_ = {MAP_WIDTH/2, 0};
    enemy_advance_ = true;
    level_ = level;
    srand(time(0));

    for (char i = 0; i < MAP_HEIGHT; i++) {
        for (char j = 0; j < MAP_WIDTH; j++) {
            char tile;
            int rng = rand() % 1000;
            
            if (j >= player_pos_ - 4 && j < player_pos_ + car_width + 4 && i > MAP_HEIGHT - car_height - 6) {
                tile = tile_empty;
            }
            else {
                tile = generate_tile(j, rng);
            }

            playfield_[i][j] = tile;
        }
    }
}

char Map::generate_tile(char row, int rng) {
    char tile;
    if (row == 0 || row == MAP_WIDTH - 1) {
        tile = tile_border;
    }
    else if (rng < 8 + 3*level_) {
        tile = tile_spikes;
    }
    else if (rng < 11 + 3*level_) {
        tile = tile_bonus;
    }
    else if (rng < 12 + 3*level_) {
        tile = tile_special;
    }
    else {
        tile = tile_empty;
    }

    return tile;
}

char Map::player_collision() {
    for (char i = MAP_HEIGHT - 1; i > MAP_HEIGHT - car_height - 1; i--) {
        for (char j = player_pos_; j < player_pos_ + car_width; j++) {
            if (playfield_[i][j] == tile_special) {
                clear_grid();
                return 0;
            }
            
            else if (playfield_[i][j] != tile_empty) {
                char collision = playfield_[i][j];
                playfield_[i][j] = tile_empty;
                return collision;
            }

            if (enemy_pos_.Y > MAP_HEIGHT - 2*car_height && enemy_pos_.X > player_pos_ - car_width && enemy_pos_.X < player_pos_ + car_width) {
                enemy_pos_.X = MAP_WIDTH/2;
                enemy_pos_.Y = 0;
                return 'E';
            }
        }
    }
    return 0;
}

void Map::enemy_car_movement() {
    if (enemy_advance_) {
        enemy_advance_ = false;
        int lateral_movement = rand() % 3;

        if (lateral_movement == 1) {
            enemy_pos_.X += 1;
        }
        else if (lateral_movement == 2) {
            enemy_pos_.X -= 1;
        }

        if (enemy_pos_.X < 1) {
            enemy_pos_.X = 1;
        }
        else if (enemy_pos_.X > MAP_WIDTH - car_width - 1) {
            enemy_pos_.X = MAP_WIDTH - car_width - 1;
        }
    }

    else {
        enemy_advance_ = true;
        enemy_pos_.Y += 1;
        
        if (enemy_pos_.Y > MAP_HEIGHT - car_height) {
            enemy_pos_.X = MAP_WIDTH/2;
            enemy_pos_.Y = 0;
        }
    }
}

bool Map::player_move_sideways(int keypressed) {
    switch (keypressed) {
        case 'a':
            player_pos_ -= 1;
            break;
        case 'd':
            player_pos_ += 1;
            break;
    }

    if (player_pos_ < 1) {
        player_pos_ = 1;
        return true;
    }
    else if (player_pos_ > MAP_WIDTH - car_width - 1) {
        player_pos_ = MAP_WIDTH - car_width - 1;
        return true;
    }

    return false;
}

void Map::advance() {
    enemy_car_movement();

    for (char i = MAP_HEIGHT - 1; i > 0; i--) {
        for (char j = 0; j < MAP_WIDTH; j++) {
            playfield_[i][j] = playfield_[i - 1][j];
            if (j >= enemy_pos_.X && j < enemy_pos_.X + car_width && i < enemy_pos_.Y + car_height && i >= enemy_pos_.Y) {
                playfield_[i][j] = tile_empty;
            }
        }
    }
    
    for (char j = 0; j < MAP_WIDTH; j++) {
        int rng = rand() % 1000;
        char tile;
        
        if (j == 0 || j == MAP_WIDTH - 1) {
            tile = tile_border;
        }
        else if (enemy_pos_.Y == 0 && enemy_pos_.X >= j && enemy_pos_.X < j + car_width) {
            tile = tile_empty;
        }
        else {
            tile = generate_tile(j, rng);
        }

        playfield_[0][j] = tile;
        
    }

}

void Map::draw(CHAR_INFO screen_grid[]) {
    for (char i = 0; i < MAP_HEIGHT; i++) {
        for (char j = 0; j < MAP_WIDTH; j++) {
            _COORD cell = {j, i};

            if (playfield_[i][j] == tile_spikes) {
                screen_grid[j + i * SCREEN_WIDTH].Char.AsciiChar = 'X';
                screen_grid[j + i * SCREEN_WIDTH].Attributes = FOREGROUND_RED;
            }
            else if (playfield_[i][j] == tile_bonus) {
                screen_grid[j + i * SCREEN_WIDTH].Char.AsciiChar = 'B';
                screen_grid[j + i * SCREEN_WIDTH].Attributes = FOREGROUND_GREEN;
            }
            else if (playfield_[i][j] == tile_special) {
                screen_grid[j + i * SCREEN_WIDTH].Char.AsciiChar = 'S';
                screen_grid[j + i * SCREEN_WIDTH].Attributes = FOREGROUND_BLUE;
            }
            else if (playfield_[i][j] == tile_border) {
                screen_grid[j + i * SCREEN_WIDTH].Char.AsciiChar = '#';
                screen_grid[j + i * SCREEN_WIDTH].Attributes = FOREGROUND_WHITE;
            }
        }
    }

    for (char i = MAP_HEIGHT - 1; i > MAP_HEIGHT - car_height - 1; i--) {
        for (char j = player_pos_; j < player_pos_ + car_width; j++) {
            screen_grid[j + i * SCREEN_WIDTH].Char.AsciiChar = 'P';
            screen_grid[j + i * SCREEN_WIDTH].Attributes = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        }
    }

    for (char i = enemy_pos_.Y; i < enemy_pos_.Y + car_height; i++) {
        for (char j = enemy_pos_.X; j < enemy_pos_.X + car_width; j++) {
            screen_grid[j + i * SCREEN_WIDTH].Char.AsciiChar = 'E';
            screen_grid[j + i * SCREEN_WIDTH].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; 
        }
    }
}

void Map::clear_grid() {
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 1; j < MAP_WIDTH - 1; j++) {
            playfield_[i][j] = tile_empty;
        }
    }
}

