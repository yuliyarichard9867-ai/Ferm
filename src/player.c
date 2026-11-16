#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "utils.h"

/* ======================= ЖИЗНЕННЫЙ ЦИКЛ ======================= */

Player *player_create(const char *name)
{
    Player *p = (Player *)malloc(sizeof(Player));
    if (p == NULL) {
        printf("Ошибка: не удалось выделить память под персонажа.\n");
        return NULL;
    }

    memset(p, 0, sizeof(Player));

    strncpy(p->name, name, MAX_FARMER_NAME - 1);
    p->name[MAX_FARMER_NAME - 1] = '\0';

    p->max_hp      = BASE_MAX_HP;
    p->max_stamina = BASE_MAX_STAMINA;
    p->hp          = p->max_hp;
    p->stamina     = p->max_stamina;
    p->gold        = START_GOLD;
    p->x = 0;
    p->y = 0;

    for (int i = 0; i < MAX_INVENTORY; i++)
        p->inventory[i] = ITEM_EMPTY;

    for (int i = 0; i < EQUIP_SLOTS; i++) {
        p->equipment[i].name[0]     = '\0';
        p->equipment[i].slot        = (enum EquipSlot)i;
        p->equipment[i].bonus_hp    = 0;
        p->equipment[i].bonus_stamina = 0;
        p->equipment[i].is_equipped = 0;
    }

    printf("Память структуры по адресу %p выделена (персонаж создан)\n",
           (void *)p);
    return p;
}

void player_destroy(Player *p)
{
    if (p == NULL) return;
    printf("Память структуры по адресу %p освобождена\n", (void *)p);
    free(p);
}

/* ======================= ЭКИПИРОВКА ======================= */

void player_recalc_stats(Player *p)
{
    if (p == NULL) return;

    int new_max_hp      = BASE_MAX_HP;
    int new_max_stamina = BASE_MAX_STAMINA;

    for (int i = 0; i < EQUIP_SLOTS; i++) {
        if (p->equipment[i].is_equipped) {
            new_max_hp      += p->equipment[i].bonus_hp;
            new_max_stamina += p->equipment[i].bonus_stamina;
        }
    }

    p->max_hp      = new_max_hp;
    p->max_stamina = new_max_stamina;

    if (p->hp      > p->max_hp)      p->hp      = p->max_hp;
    if (p->stamina > p->max_stamina) p->stamina = p->max_stamina;
}

int player_equip_item(Player *p, const Equipment *item)
{
    if (p == NULL || item == NULL) return 0;
    if (item->slot < 0 || item->slot >= EQUIP_SLOTS) return 0;

    p->equipment[item->slot] = *item;
    p->equipment[item->slot].is_equipped = 1;

    player_recalc_stats(p);
    return 1;
}

int player_unequip_slot(Player *p, enum EquipSlot slot)
{
    if (p == NULL || slot < 0 || slot >= EQUIP_SLOTS) return 0;
    if (!p->equipment[slot].is_equipped) return 0;

    p->equipment[slot].is_equipped = 0;
    player_recalc_stats(p);
    return 1;
}

void player_show_equipment(const Player *p)
{
    if (p == NULL) return;

    static const char *slot_names[EQUIP_SLOTS] = { "Голова", "Тело", "Обувь" };

    printf("\n--- Экипировка ---\n");
    for (int i = 0; i < EQUIP_SLOTS; i++) {
        printf("  [%s] ", slot_names[i]);
        if (p->equipment[i].is_equipped)
            printf("%s (+%d HP, +%d ST)\n",
                   p->equipment[i].name,
                   p->equipment[i].bonus_hp,
                   p->equipment[i].bonus_stamina);
        else
            printf("— пусто —\n");
    }
    printf("  Итого: max_hp=%d, max_stamina=%d\n",
           p->max_hp, p->max_stamina);
}

/* ======================= ИНВЕНТАРЬ ======================= */

void player_print_inventory(const Player *p)
{
    if (p == NULL) return;

    printf("Инвентарь:\n");
    for (int i = 0; i < MAX_INVENTORY; i++) {
        int id = p->inventory[i];
        if (id == ITEM_EMPTY)
            printf("Слот %d: [0] (Пусто)\n", i);
        else
            printf("Слот %d: [%d] (%s)\n", i, id, item_name(id));
    }
}

void player_add_item_to_slot(Player *p, int slot, int id)
{
    if (p == NULL) return;
    if (slot < 0 || slot >= MAX_INVENTORY) return;
    if (id < 0 || id >= MAX_ITEMS) return;

    p->inventory[slot] = id;
    printf("В слот %d положен предмет [%d] (%s).\n",
           slot, id, item_name(id));
}

void player_clear_slot(Player *p, int slot)
{
    if (p == NULL) return;
    if (slot < 0 || slot >= MAX_INVENTORY) return;

    p->inventory[slot] = ITEM_EMPTY;
    printf("Слот %d очищен.\n", slot);
}

void player_check_neighbours(const Player *p, int id1, int id2)
{
    if (p == NULL) return;

    int found = 0;
    for (int i = 0; i < MAX_INVENTORY - 1; i++) {
        int a = p->inventory[i];
        int b = p->inventory[i + 1];
        if ((a == id1 && b == id2) || (a == id2 && b == id1)) {
            printf("Да! Предметы [%d] (%s) и [%d] (%s) лежат рядом "
                   "в слотах %d и %d.\n",
                   id1, item_name(id1), id2, item_name(id2), i, i + 1);
            found = 1;
        }
    }
    if (!found)
        printf("Нет. Предметы [%d] (%s) и [%d] (%s) не лежат рядом.\n",
               id1, item_name(id1), id2, item_name(id2));
}

void player_search_item(const Player *p, const char *query)
{
    if (p == NULL || query == NULL) return;

    int id = find_item_id_by_name(query);
    if (id < 0) {
        printf("Предмет '%s' не найден в каталоге.\n", query);
        return;
    }

    int found = 0;
    for (int i = 0; i < MAX_INVENTORY; i++) {
        if (p->inventory[i] == id) {
            printf("Найден в слоте %d: [%d] - %s\n", i, id, item_name(id));
            found++;
        }
    }

    if (found == 0)
        printf("Предмет '%s' (ID %d) есть в каталоге, но в рюкзаке его нет.\n",
               query, id);
    else
        printf("Всего найдено: %d шт.\n", found);
}

/* ======================= КАРТОЧКА ПЕРСОНАЖА ======================= */

void player_print_card(const Player *p)
{
    if (p == NULL) {
        printf("Персонаж не создан!\n");
        return;
    }

    printf("\n========== КАРТОЧКА ПЕРСОНАЖА ==========\n");
    printf("Имя:           %s\n", p->name);
    printf("HP:            %d/%d\n", p->hp, p->max_hp);
    printf("Выносливость:  %d/%d\n", p->stamina, p->max_stamina);
    printf("Золото:        %d\n", p->gold);
    printf("Координаты:    (%d, %d)\n", p->x, p->y);
    printf("========================================\n");
    player_show_equipment(p);
    printf("----------------------------------------\n");
    player_print_inventory(p);
    printf("========================================\n\n");
}