#include <stdio.h>
#include <stdlib.h>
#include "map.h"

GameState *game_create(void)
{
    GameState *g = (GameState *)malloc(sizeof(GameState));
    if (g == NULL) {
        printf("Ошибка: не удалось выделить память под GameState.\n");
        return NULL;
    }
    g->current_day  = MAP_START_DAY;
    g->current_hour = MAP_START_HOUR;

    printf("Память структуры по адресу %p выделена (время инициализировано)\n",
           (void *)g);
    return g;
}

void game_destroy(GameState *g)
{
    if (g == NULL) return;
    printf("Память структуры по адресу %p освобождена\n", (void *)g);
    free(g);
}

void game_show_time(const GameState *g)
{
    if (g == NULL) return;
    printf("Текущее время: День %d, %02d:00\n", g->current_day, g->current_hour);
}

void game_skip_time(GameState *g, int hours)
{
    if (g == NULL) return;

    g->current_hour += hours;
    while (g->current_hour >= 24) {
        g->current_hour -= 24;
        g->current_day  += 1;
    }
    printf("Прошло %d ч. Теперь: День %d, %02d:00\n",
           hours, g->current_day, g->current_hour);
}