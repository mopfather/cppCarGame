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
        char player_collision();
        //returns 0 if no collision occured
        //returns the type of first object car collided with as char otherwise
        //clears the cell of the object returned
        void enemy_car_movement();
        bool player_move_sideways(int keypressed);
        //updates player position based on input
        //returns true if wall was hit
        void advance();
        void draw(HANDLE screen_buffer);
};