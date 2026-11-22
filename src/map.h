#ifndef MAP_H
#define MAP_H

#include <stdio.h>

/* ======================= КОНСТАНТЫ ВРЕМЕНИ ======================= */
#define MAP_START_DAY        1
#define MAP_START_HOUR       8
#define NIGHT_HOUR           2

/* ======================= КОНСТАНТЫ КАРТЫ ======================= */
#define MAP_MIN_SIZE         5
#define MAP_MAX_SIZE         30
#define MAP_WALL_PERCENT_MIN 15
#define MAP_WALL_PERCENT_MAX 30
#define OBSTACLE_TREE_PCT    50
#define OBSTACLE_ROCK_PCT    50

#define FLOOR_GOLD_MULT      10
#define FLOOR_STAMINA_COST   5
#define STEP_STAMINA_COST    1
#define EVAC_GOLD_PERCENT    50
#define MAX_FLOOR            20

/* ======================= НОЧНОЙ ЛЕС ======================= */
#define SHOUT_RADIUS         6      /* радиус раскрытия (5..7) */
#define SHOUT_DURATION       3      /* на сколько шагов раскрывается */

/* ======================= ТИПЫ КЛЕТОК ======================= */
enum CellType {
    CELL_EMPTY = '.',
    CELL_TREE = 'T',
    CELL_ROCK = 'R',
    CELL_CAMP = '^',     /* лагерь/костёр — выход домой */
    CELL_PLAYER = '@',
    CELL_UNKNOWN = '?'      /* скрытая клетка */
};

/* ======================= НАПРАВЛЕНИЯ ======================= */
enum Direction {
    DIR_NONE = 0,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_QUIT,
    DIR_SHOUT           /* крик — раскрыть препятствия вокруг */
};

/* ======================= СТРУКТУРЫ ======================= */
typedef struct Map {
    char** cells;           /* реальная карта */
    char** visible;         /* что видит игрок: '?' или реальный символ */
    char** revealed;        /* 1 = клетка раскрыта криком, 0 = скрыта */
    int width;
    int height;
    int floor;              /* «дальность от дома» */
    int camp_x, camp_y;     /* координаты лагеря */
    int shout_timer;        /* сколько шагов ещё действует крик */
} Map;

typedef struct GameState {
    int current_day;
    int current_hour;
    Map* map;
} GameState;

/* ======================= ВРЕМЯ ======================= */
GameState* game_create(void);
void       game_destroy(GameState* g);
void       game_show_time(const GameState* g);
void       game_skip_time(GameState* g, int hours);
void       game_sleep(GameState* g);

/* ======================= КАРТА ======================= */
Map* map_create(int width, int height, int floor);
void  map_destroy(Map* m);
void  map_generate(Map* m);
void  map_update_visible(Map* m);
void  map_print_visible(const Map* m);
int   random_map_size(void);

/* ======================= ПРИКЛЮЧЕНИЕ ======================= */
struct Player;
void adventure_run(GameState* game, struct Player* player);

#endif /* MAP_H */