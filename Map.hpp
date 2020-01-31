#include <windows.h>

#define MAP_WIDTH 47
#define MAP_HEIGHT 22

struct Map_position {
    char X;
    char Y;
};

class Map {
    private:
        char grid_[MAP_HEIGHT][MAP_WIDTH];
        char player_pos_;
        Map_position enemy_pos_;
        bool enemy_advance_;
        int level_;
 
    public:
        const char car_width = 2;
        const char car_height = 2;
        Map(int level);
        char generate_tile(char col, int rng);
        bool player_move_sideways(int keypressed);
        void advance();
        char player_collision();
        void enemy_car_movement();
        void draw(HANDLE screen_buffer);
        void clear_spikes();
};