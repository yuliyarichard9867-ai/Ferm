#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "entity.h"
#include "player.h"
#include "utils.h"

#define ENTITY_PERCENT       25
#define MONSTER_PERCENT      20
#define HEAL_PERCENT         10
#define TREE_HP_BASE         10
#define ROCK_HP_BASE         20
#define MONSTER_HP_BASE      15
#define MONSTER_ATK_BASE     4
#define MONSTER_DEF_BASE     1
#define MONSTER_GOLD_BASE    5
#define FLOOR_MULT           2
#define INTERACT_STAMINA     2

Entity* entities_create(int count)
{
    if (count <= 0) return NULL;
    Entity* arr = (Entity*)malloc(count * sizeof(Entity));
    if (arr == NULL) {
        printf("Ошибка: не удалось выделить память под сущности.\n");
        return NULL;
    }
    printf("Память массива сущностей по адресу %p выделена (%d шт.)\n",
        (void*)arr, count);
    return arr;
}

void entities_destroy(Entity* entities, int count)
{
    (void)count;
    if (entities == NULL) return;
    printf("Память массива сущностей по адресу %p освобождена\n",
        (void*)entities);
    free(entities);
}

static int rand_range(int min, int max)
{
    return min + rand() % (max - min + 1);
}

int entities_generate(Entity* entities, int max_count,
    int map_w, int map_h, int floor)
{
    if (entities == NULL || max_count <= 0) return 0;

    int inner_cells = map_w * map_h;
    int target = inner_cells * ENTITY_PERCENT / 100;
    if (target > max_count) target = max_count;

    int created = 0;
    for (int i = 0; i < target; i++) {
        int x = rand_range(1, map_w - 2);
        int y = rand_range(1, map_h - 2);

        if (x == 1 && y == 1) continue;

        int busy = 0;
        for (int k = 0; k < created; k++)
            if (entities[k].is_alive &&
                entities[k].x == x && entities[k].y == y) {
                busy = 1; break;
            }
        if (busy) continue;

        Entity* e = &entities[created];
        memset(e, 0, sizeof(Entity));
        e->x = x;
        e->y = y;
        e->is_alive = 1;

        int roll = rand_range(1, 100);
        if (roll <= MONSTER_PERCENT) {
            e->type = ENT_MONSTER;
            e->symbol = 'M';
            e->hp = e->max_hp = MONSTER_HP_BASE * floor * FLOOR_MULT;
            e->attack = MONSTER_ATK_BASE + floor;
            e->defense = MONSTER_DEF_BASE + floor / 2;
            e->gold_reward = MONSTER_GOLD_BASE * floor;
        }
        else if (roll <= MONSTER_PERCENT + HEAL_PERCENT) {
            e->type = ENT_HEAL;
            e->symbol = '+';
            e->hp = e->max_hp = 1;
        }
        else {
            if (rand() % 2 == 0) {
                e->type = ENT_TREE;
                e->symbol = 'T';
                e->hp = e->max_hp = TREE_HP_BASE * floor;
                e->drop_item_id = 1;
            }
            else {
                e->type = ENT_ROCK;
                e->symbol = 'R';
                e->hp = e->max_hp = ROCK_HP_BASE * floor;
                e->drop_item_id = 2;
            }
        }
        created++;
    }
    return created;
}

Entity* entity_at(Entity* entities, int count, int x, int y)
{
    if (entities == NULL) return NULL;
    for (int i = 0; i < count; i++) {
        if (entities[i].is_alive &&
            entities[i].x == x && entities[i].y == y)
            return &entities[i];
    }
    return NULL;
}

static int add_to_inventory(Player* p, int item_id)
{
    for (int i = 0; i < MAX_INVENTORY; i++) {
        if (p->inventory[i] == ITEM_EMPTY) {
            p->inventory[i] = item_id;
            return 1;
        }
    }
    return 0;
}

static void check_shield(Player* p, char* event_buf, int buf_size)
{
    if (p->shield_used || p->shield_active) return;
    if (p->hp >= p->max_hp / 2) return;

    p->shield_active = 1;
    p->shield_used = 1;

    size_t len = strlen(event_buf);
    if (len + 60 < (size_t)buf_size) {
        snprintf(event_buf + len, buf_size - len,
            " МАГИЧЕСКИЙ ЩИТ активирован!");
    }
}

int entity_interact(Player* player, Entity* e,
    char* event_buf, int buf_size, int floor)
{
    if (player == NULL || e == NULL || !e->is_alive) return 0;

    player->stamina -= INTERACT_STAMINA;
    if (player->stamina < 0) player->stamina = 0;

    if (e->type == ENT_TREE || e->type == ENT_ROCK) {
        int dmg = player->attack - e->defense / 2;
        if (dmg < 1) dmg = 1;
        e->hp -= dmg;

        if (e->hp <= 0) {
            e->is_alive = 0;
            const char* name = item_name(e->drop_item_id);
            if (add_to_inventory(player, e->drop_item_id)) {
                snprintf(event_buf, buf_size,
                    "Вы добыли %s! (урон %d)", name, dmg);
            }
            else {
                snprintf(event_buf, buf_size,
                    "Рюкзак полон! %s пропал.", name);
            }
        }
        else {
            snprintf(event_buf, buf_size,
                "Удар по %s: -%d HP (осталось %d/%d)",
                e->type == ENT_TREE ? "дереву" : "камню",
                dmg, e->hp, e->max_hp);
        }
        return 1;
    }

    if (e->type == ENT_HEAL) {
        int heal_hp = 10 + floor * 2;
        int heal_st = 5 + floor;
        player->hp += heal_hp;
        if (player->hp > player->max_hp) player->hp = player->max_hp;
        player->stamina += heal_st;
        if (player->stamina > player->max_stamina)
            player->stamina = player->max_stamina;

        e->is_alive = 0;
        snprintf(event_buf, buf_size,
            "Аптечка! +%d HP, +%d стамины", heal_hp, heal_st);
        return 1;
    }

    if (e->type == ENT_MONSTER) {
        int dmg = player->attack - e->defense / 2;
        if (dmg < 1) dmg = 1;
        e->hp -= dmg;

        if (e->hp <= 0) {
            e->is_alive = 0;
            player->gold += e->gold_reward;
            snprintf(event_buf, buf_size,
                "Монстр побеждён! +%d золота", e->gold_reward);
            return 1;
        }

        int mdmg = e->attack - player->defense / 2;
        if (mdmg < 1) mdmg = 1;

        if (player->shield_active) {
            player->shield_active = 0;
            snprintf(event_buf, buf_size,
                "Бой: вы -%d монстру. ЩИТ заблокировал удар! "
                "(у монстра %d/%d HP)",
                dmg, e->hp, e->max_hp);
            return 1;
        }

        player->hp -= mdmg;
        snprintf(event_buf, buf_size,
            "Бой: вы -%d монстру, он -%d вам (у монстра %d/%d HP)",
            dmg, mdmg, e->hp, e->max_hp);

        check_shield(player, event_buf, buf_size);
        return 1;
    }

    return 0;
}