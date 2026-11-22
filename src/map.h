#ifndef MAP_H
#define MAP_H

#include <stdio.h>
#include "entity.h"

#define MAP_START_DAY        1
#define MAP_START_HOUR       8
#define NIGHT_HOUR           2

#define MAP_MIN_SIZE         5
#define MAP_MAX_SIZE         30
#define MAP_WALL_PERCENT_MIN 15
#define MAP_WALL_PERCENT_MAX 30

#define FLOOR_GOLD_MULT      10
#define FLOOR_STAMINA_COST   5
#define STEP_STAMINA_COST    1
#define EVAC_GOLD_PERCENT    50
#define MAX_FLOOR            20

#define MAX_ENTITIES         300
#define SHOUT_RADIUS         6
#define SHOUT_DURATION       3

enum CellType {
    CELL_EMPTY = '.',
    CELL_CAMP = '^',
    CELL_PLAYER = '@',
    CELL_UNKNOWN = '?'
};

enum Direction {
    DIR_NONE = 0,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_QUIT,
    DIR_SHOUT
};

#define MOVE_BLOCKED   0
#define MOVE_OK        1
#define MOVE_EDGE      2
#define MOVE_INTERACT  3

typedef struct Map {
    char** cells;
    char** visible;
    char** revealed;
    int width, height, floor;
    int camp_x, camp_y;
    int shout_timer;
    Entity* entities;
    int entity_count;
} Map;

typedef struct GameState {
    int current_day;
    int current_hour;
    Map* map;
} GameState;

GameState* game_create(void);
void       game_destroy(GameState* g);
void       game_show_time(const GameState* g);
void       game_skip_time(GameState* g, int hours);
void       game_sleep(GameState* g);

Map* map_create(int width, int height, int floor);
void  map_destroy(Map* m);
void  map_generate(Map* m);
void  map_update_visible(Map* m);
void  map_print_visible(const Map* m);
int   random_map_size(void);

struct Player;
void adventure_run(GameState* game, struct Player* player);

#endif /* MAP_H */
