#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "map.h"
#include "player.h"
#include "utils.h"

/* ======================= ВРЕМЯ ======================= */

GameState* game_create(void)
{
    GameState* g = (GameState*)malloc(sizeof(GameState));
    if (g == NULL) return NULL;
    g->current_day = MAP_START_DAY; 
    g->current_hour = MAP_START_HOUR;
    g->map = NULL;
    printf("Память структуры по адресу %p выделена (GameState)\n", (void*)g);
    return g;
}

void game_destroy(GameState* g)
{
    if (g == NULL) return;
    if (g->map != NULL) { map_destroy(g->map); g->map = NULL; }
    printf("Память структуры по адресу %p освобождена (GameState)\n", (void*)g);
    free(g);
}

void game_show_time(const GameState* g)
{
    if (g == NULL) return;
    printf("Текущее время: День %d, %02d:00\n", g->current_day, g->current_hour);
}

void game_skip_time(GameState* g, int hours)
{
    if (g == NULL || hours < 0) return;
    g->current_hour += hours;
    while (g->current_hour >= 24) {
        g->current_hour -= 24;
        g->current_day += 1;
    }
    printf("Прошло %d ч. Теперь: День %d, %02d:00\n",
        hours, g->current_day, g->current_hour);
}

void game_sleep(GameState* g)
{
    if (g == NULL) return;
    g->current_day += 1;
    g->current_hour = MAP_START_HOUR;
    printf("Герой выспался. День %d, %02d:00.\n",
        g->current_day, g->current_hour);
}

/* ======================= КАРТА: ПАМЯТЬ ======================= */

int random_map_size(void)
{
    return MAP_MIN_SIZE + rand() % (MAP_MAX_SIZE - MAP_MIN_SIZE + 1);
}

Map* map_create(int width, int height, int floor)
{
    if (width < MAP_MIN_SIZE || height < MAP_MIN_SIZE ||
        width > MAP_MAX_SIZE || height > MAP_MAX_SIZE) {
        printf("Ошибка: размер карты должен быть от %d до %d.\n",
            MAP_MIN_SIZE, MAP_MAX_SIZE);
        return NULL;
    }

    Map* m = (Map*)malloc(sizeof(Map));
    if (m == NULL) return NULL;

    m->width = width; m->height = height; m->floor = floor;
    m->camp_x = -1; m->camp_y = -1;
    m->shout_timer = 0;
    m->entities = NULL; m->entity_count = 0;

    m->cells = (char**)malloc(height * sizeof(char*));
    m->visible = (char**)malloc(height * sizeof(char*));
    m->revealed = (char**)malloc(height * sizeof(char*));
    if (!m->cells || !m->visible || !m->revealed) {
        free(m->cells); free(m->visible); free(m->revealed); free(m);
        return NULL;
    }

    for (int y = 0; y < height; y++) {
        m->cells[y] = (char*)malloc(width * sizeof(char));
        m->visible[y] = (char*)malloc(width * sizeof(char));
        m->revealed[y] = (char*)malloc(width * sizeof(char));
        if (!m->cells[y] || !m->visible[y] || !m->revealed[y]) {
            for (int k = 0; k <= y; k++) {
                free(m->cells[k]); free(m->visible[k]); free(m->revealed[k]);
            }
            free(m->cells); free(m->visible); free(m->revealed); free(m);
            return NULL;
        }
    }

    printf("Память карты по адресу %p выделена (этаж %d, размер %dx%d)\n",
        (void*)m, floor, width, height);
    return m;
}

void map_destroy(Map* m)
{
    if (m == NULL) return;

    if (m->entities != NULL) {
        entities_destroy(m->entities, m->entity_count);
        m->entities = NULL;
    }

    for (int y = 0; y < m->height; y++) {
        free(m->cells[y]); free(m->visible[y]); free(m->revealed[y]);
    }
    free(m->cells); free(m->visible); free(m->revealed);
    printf("Память карты по адресу %p освобождена\n", (void*)m);
    free(m);
}

/* ======================= КАРТА: ГЕНЕРАЦИЯ ======================= */

void map_generate(Map* m)
{
    if (m == NULL) return;

    for (int y = 0; y < m->height; y++)
        for (int x = 0; x < m->width; x++) {
            m->cells[y][x] = CELL_EMPTY;
            m->revealed[y][x] = 0;
        }

    m->cells[1][1] = CELL_EMPTY;

    int placed = 0;
    for (int tries = 0; tries < 100 && !placed; tries++) {
        int x = rand() % m->width;
        int y = rand() % m->height;
        if (m->cells[y][x] == CELL_EMPTY && !(x == 1 && y == 1)) {
            m->cells[y][x] = CELL_CAMP;
            m->camp_x = x; m->camp_y = y;
            placed = 1;
        }
    }
    if (!placed) {
        int x = m->width - 2, y = m->height - 2;
        if (x >= 0 && y >= 0) {
            m->cells[y][x] = CELL_CAMP;
            m->camp_x = x; m->camp_y = y;
        }
    }

    m->shout_timer = 0;

    m->entities = entities_create(MAX_ENTITIES);
    if (m->entities != NULL) {
        m->entity_count = entities_generate(m->entities, MAX_ENTITIES,
            m->width, m->height, m->floor);
    }

    map_update_visible(m);
}

void map_update_visible(Map* m)
{
    if (m == NULL) return;

    /* 1. Заполняем visible: скрытое — '?', раскрытое — реальный символ */
    for (int y = 0; y < m->height; y++)
        for (int x = 0; x < m->width; x++)
            m->visible[y][x] = m->revealed[y][x] ? m->cells[y][x] : CELL_UNKNOWN;

    /* 2. Накладываем сущности ТОЛЬКО на раскрытые клетки */
    for (int i = 0; i < m->entity_count; i++) {
        Entity* e = &m->entities[i];
        if (!e->is_alive) continue;
        if (e->x < 0 || e->x >= m->width)  continue;
        if (e->y < 0 || e->y >= m->height) continue;

        /* Если клетка не раскрыта криком — сущность скрыта */
        if (!m->revealed[e->y][e->x]) continue;

        m->visible[e->y][e->x] = e->symbol;
    }
}

void map_print_visible(const Map* m)
{
    if (m == NULL) return;
    for (int y = 0; y < m->height; y++) {
        for (int x = 0; x < m->width; x++) putchar(m->visible[y][x]);
        putchar('\n');
    }
}

/* ======================= КРИК ======================= */

static void do_shout(Map* m, int px, int py)
{
    if (m == NULL) return;
    m->shout_timer = SHOUT_DURATION;
    for (int dy = -SHOUT_RADIUS; dy <= SHOUT_RADIUS; dy++)
        for (int dx = -SHOUT_RADIUS; dx <= SHOUT_RADIUS; dx++) {
            if (dx * dx + dy * dy > SHOUT_RADIUS * SHOUT_RADIUS) continue;
            int x = px + dx, y = py + dy;
            if (x < 0 || x >= m->width || y < 0 || y >= m->height) continue;
            m->revealed[y][x] = 1;
        }
    printf("Вы крикнули! Препятствия вокруг раскрыты на %d шага(ов).\n",
        SHOUT_DURATION);
    map_update_visible(m);
}

/* ======================= HUD ======================= */

static void print_hud(const Player* p, const Map* m, const GameState* g)
{
    printf("HP: %d/%d | ST: %d/%d | Золото: %d | Этаж: %d | Время: %02d:00",
        p->hp, p->max_hp, p->stamina, p->max_stamina, p->gold,
        m->floor, g->current_hour);
    if (p->shield_active) printf(" | ЩИТ");
    printf("\n");
}

static int is_night(const GameState* g) { return g->current_hour == NIGHT_HOUR; }

/* ======================= ПЕРЕМЕЩЕНИЕ ======================= */

static int try_move(Player* p, Map* m, enum Direction dir,
    char* event_buf, int buf_size)
{
    int nx = p->x, ny = p->y;
    switch (dir) {
    case DIR_UP:    ny -= 1; break;
    case DIR_DOWN:  ny += 1; break;
    case DIR_LEFT:  nx -= 1; break;
    case DIR_RIGHT: nx += 1; break;
    default: return MOVE_BLOCKED;
    }

    if (nx < 0 || nx >= m->width || ny < 0 || ny >= m->height)
        return MOVE_EDGE;

    Entity* e = entity_at(m->entities, m->entity_count, nx, ny);
    if (e != NULL && e->is_alive) {
        entity_interact(p, e, event_buf, buf_size, m->floor);
        return MOVE_INTERACT;
    }

    p->x = nx; p->y = ny;
    p->stamina -= STEP_STAMINA_COST;
    if (p->stamina < 0) p->stamina = 0;
    return MOVE_OK;
}

/* ======================= ЭВАКУАЦИЯ ======================= */

static void emergency_evac(Player* p, GameState* g, const char* reason)
{
    printf("\n!!! %s !!!\n", reason);
    printf("Спасательная служба доставляет героя домой...\n");
    int fee = p->gold * EVAC_GOLD_PERCENT / 100;
    p->gold -= fee;
    if (p->gold < 0) p->gold = 0;
    printf("За услуги списано %d золота. Остаток: %d.\n", fee, p->gold);
    if (g->map != NULL) { map_destroy(g->map); g->map = NULL; }
    p->stamina = 0;
}

/* ======================= ВВОД ======================= */

static enum Direction ask_direction(void)
{
    char buf[8];
    printf("Ход (w/a/s/d, r-крик, q-выход): ");
    if (read_line(buf, sizeof(buf)) != OK || buf[0] == '\0') return DIR_NONE;
    switch (buf[0]) {
    case 'w': case 'W': return DIR_UP;
    case 's': case 'S': return DIR_DOWN;
    case 'a': case 'A': return DIR_LEFT;
    case 'd': case 'D': return DIR_RIGHT;
    case 'q': case 'Q': return DIR_QUIT;
    case 'r': case 'R': return DIR_SHOUT;
    default:            return DIR_NONE;
    }
}

/* ======================= ГЛАВНЫЙ ЦИКЛ ======================= */

void adventure_run(GameState* game, Player* player)
{
    if (game == NULL || player == NULL) return;
    if (player->stamina == 0) { printf("Нельзя отправиться: выносливость 0.\n"); return; }
    if (is_night(game)) { printf("Нельзя отправиться: 02:00 ночи.\n"); return; }

    game->map = map_create(random_map_size(), random_map_size(), 1);
    if (game->map == NULL) return;
    map_generate(game->map);
    player->x = 1; player->y = 1;

    player_reset_shield(player);

    char last_event[128] = "Приключение началось.";

    printf("\n=== НОЧНОЙ ЛЕС ===\n");
    printf("R — крик. Ищите ^ (лагерь), Q — выйти домой.\n");

    int running = 1;
    while (running) {
        /* ---------- RENDER ---------- */
        printf("\n");
        print_hud(player, game->map, game);
        printf("Последнее событие: %s\n", last_event);

        char saved = game->map->cells[player->y][player->x];
        game->map->cells[player->y][player->x] = CELL_PLAYER;
        map_update_visible(game->map);
        game->map->visible[player->y][player->x] = CELL_PLAYER;
        map_print_visible(game->map);
        game->map->cells[player->y][player->x] = saved;

        /* ---------- INPUT ---------- */
        enum Direction dir = ask_direction();

        /* ---------- UPDATE ---------- */
        if (dir == DIR_SHOUT) { do_shout(game->map, player->x, player->y); continue; }
        if (dir == DIR_QUIT) {
            if (player->x == game->map->camp_x && player->y == game->map->camp_y) {
                printf("Вы вернулись домой. Золото: %d.\n", player->gold);
                write_diary(player->name, game->current_day, game->current_hour,
                    player->inventory, MAX_INVENTORY);
                map_destroy(game->map); game->map = NULL; running = 0;
            }
            else printf("Выйти можно только на клетке лагеря ^.\n");
            continue;
        }
        if (dir == DIR_NONE) continue;

        int moved = try_move(player, game->map, dir, last_event, sizeof(last_event));

        if (moved == MOVE_EDGE) {
            int new_floor = game->map->floor + 1;
            if (new_floor > MAX_FLOOR) { printf("Максимальный этаж.\n"); continue; }
            map_destroy(game->map);
            game->map = map_create(random_map_size(), random_map_size(), new_floor);
            if (game->map == NULL) { running = 0; continue; }
            map_generate(game->map);
            player->x = 1; player->y = 1;
            player->stamina -= FLOOR_STAMINA_COST;
            if (player->stamina < 0) player->stamina = 0;
            game->current_hour += 1;
            if (game->current_hour >= 24) { game->current_hour -= 24; game->current_day += 1; }
            int reward = FLOOR_GOLD_MULT * new_floor;
            player->gold += reward;
            player_reset_shield(player);
            printf("Новая карта, дальность: %d. +%d золота. Щит сброшен.\n",
                new_floor, reward);
            continue;
        }

        if (moved == MOVE_OK || moved == MOVE_INTERACT) {
            if (game->map->shout_timer > 0) {
                game->map->shout_timer -= 1;
                if (game->map->shout_timer == 0) {
                    for (int y = 0; y < game->map->height; y++)
                        for (int x = 0; x < game->map->width; x++)
                            game->map->revealed[y][x] = 0;
                    printf("Тьма снова сгущается: препятствия скрыты.\n");
                }
                map_update_visible(game->map);
            }

            if (player->stamina == 0) {
                emergency_evac(player, game, "Герой падает без сил (stamina = 0)");
                running = 0; continue;
            }
            if (player->hp <= 0) {
                emergency_evac(player, game, "Герой погиб в бою");
                running = 0; continue;
            }
            if (is_night(game)) {
                emergency_evac(player, game, "Наступило 02:00 — герой засыпает");
                running = 0; continue;
            }
        }
    }

    if (game->map != NULL) { map_destroy(game->map); game->map = NULL; }
}