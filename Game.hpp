#include "Map.hpp"
#include <windows.h>

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
        HANDLE active_screen_buffer_;
        HANDLE secondary_screen_buffer_;
        HANDLE input_buffer_;
    
    public:
        Game();
        void play();
        char get_input();
        void draw_panel(HANDLE screen_buffer);
        Map* get_map(int level);
        void append_map_list(Map* new_map);
        void swap_buffers();
        void update_game_state();
};