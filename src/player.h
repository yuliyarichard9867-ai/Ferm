#ifndef PLAYER_H
#define PLAYER_H

/* ======================= КОНСТАНТЫ ======================= */
#define MAX_NAME_LEN      64
#define MAX_FARMER_NAME   33
#define MAX_ITEMS         10
#define MAX_INVENTORY     10

#define BASE_MAX_HP       100
#define BASE_MAX_STAMINA  50
#define BASE_ATTACK       5
#define BASE_DEFENSE      2
#define EQUIP_SLOTS       3
#define START_GOLD        0
#define ITEM_EMPTY        0

/* ======================= ГАРДЕРОБ ======================= */
#define PRESET_ITEMS_COUNT  6

/* ======================= ПЕРЕЧИСЛЕНИЯ ======================= */
enum EquipSlot {
    SLOT_HEAD = 0,
    SLOT_BODY,
    SLOT_FEET
};

/* ======================= СТРУКТУРЫ ======================= */
typedef struct Equipment {
    char name[MAX_NAME_LEN];
    enum EquipSlot slot;
    int bonus_hp;
    int bonus_stamina;
} Equipment;

typedef struct Player {
    char name[MAX_FARMER_NAME];
    int hp;
    int max_hp;
    int stamina;
    int max_stamina;
    int gold;
    int x, y;
    int attack;
    int defense;
    int inventory[MAX_INVENTORY];
    Equipment equipment[EQUIP_SLOTS];

    /* магический щит */
    int shield_used;
    int shield_active;
} Player;

/* ======================= ЖИЗНЕННЫЙ ЦИКЛ ======================= */
Player* player_create(const char* name);
void    player_destroy(Player* p);
void    player_reset_shield(Player* p);

/* ======================= ЭКИПИРОВКА ======================= */
void player_recalc_stats(Player* p);
int  player_equip_item(Player* p, const Equipment* item);
int  player_unequip_slot(Player* p, enum EquipSlot slot);
void player_show_equipment(const Player* p);

/* ======================= ГАРДЕРОБ ======================= */
const Equipment* player_get_preset(int index);
int              player_get_preset_count(void);

/* ======================= ИНВЕНТАРЬ ======================= */
void player_print_inventory(const Player* p);
void player_add_item_to_slot(Player* p, int slot, int id);
void player_clear_slot(Player* p, int slot);
void player_check_neighbours(const Player* p, int id1, int id2);
void player_search_item(const Player* p, const char* query);

/* ======================= КАРТОЧКА ======================= */
void player_print_card(const Player* p);

#endif /* PLAYER_H */