#include "Map.hpp"
#include <windows.h>

#define PANEL_WIDTH 19

enum class Game_state {
    running,
    paused,
    over
};

struct maps_ll {                        
    Map* map;
    maps_ll* next;
};

class Game {
    private:
        Game_state state_;
        Map* current_map_;
        maps_ll* map_list_;
        int score_;
        int level_;
        CHAR_INFO screen_grid_[SCREEN_WIDTH*MAP_HEIGHT];
        HANDLE active_screen_buffer_;
        HANDLE secondary_screen_buffer_;
        HANDLE input_buffer_;
    
    public:
        Game();
        void play();
        char get_input();
        void calculate_collisions(int wall_hit);
        void clear_screen_grid();
        void draw_panel(double fps);
        void draw_string(char* string, short attributes, int x_pos, int y_pos);
        void clear_screen_buffer();
        void render_screen_grid();
        void swap_buffers();
        void update_game_state();
        Map* get_map(int level);
        void append_map_list(Map* new_map);
        void start_screen();
        void end_screen();
};