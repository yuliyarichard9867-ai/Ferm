#ifndef ENTITY_H
#define ENTITY_H

#include <stdio.h>

enum EntityType {
    ENT_TREE = 0,
    ENT_ROCK = 1,
    ENT_HEAL = 2,
    ENT_MONSTER = 3
};

typedef struct Entity {
    enum EntityType type;
    char symbol;
    int x, y;
    int hp, max_hp;
    int attack;
    int defense;
    int drop_item_id;
    int gold_reward;
    int is_alive;
} Entity;

Entity* entities_create(int count);
void    entities_destroy(Entity* entities, int count);
int     entities_generate(Entity* entities, int max_count,
    int map_w, int map_h, int floor);
Entity* entity_at(Entity* entities, int count, int x, int y);

struct Player;
int entity_interact(struct Player* player, Entity* e,
    char* event_buf, int buf_size, int floor);

#endif /* ENTITY_H */