#ifndef MAP_H
#define MAP_H

#define MAP_START_DAY     1
#define MAP_START_HOUR    8

typedef struct GameState {
    int current_day;
    int current_hour;
    /* сюда можно добавить указатель на Map, когда будете делать приключение */
    /* struct Map *map; */
} GameState;

GameState *game_create(void);
void       game_destroy(GameState *g);

void game_show_time(const GameState *g);
void game_skip_time(GameState *g, int hours);

#endif /* MAP_H */