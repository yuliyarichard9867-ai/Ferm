#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "player.h"
#include "map.h"
#include "utils.h"

/* ======================= КОДЫ ПУНКТОВ МЕНЮ ======================= */
#define MENU_EXIT             0
#define MENU_CREATE_PLAYER    1
#define MENU_PLAYER_CARD      2
#define MENU_EQUIPMENT        3
#define MENU_TIME             4      
#define MENU_INVENTORY_MENU   5      
#define MENU_RECORDS_MENU     6      
#define MENU_ADVENTURE        7
#define MENU_MAX              7

/* ======================= ВСПОМОГАТЕЛЬНЫЕ ======================= */

static void ask_farmer_name(char* out, int size)
{
    while (1) {
        printf("Введите имя фермера (до %d символов): ", size - 1);
        if (read_line(out, size) == OK && out[0] != '\0') {
            printf("Привет, %s!\n", out);
            return;
        }
        printf("Ошибка: имя не может быть пустым и длиннее %d символов.\n",
            size - 1);
    }
}

static Player* menu_create_player(Player* old_player)
{
    if (old_player != NULL) {
        int ok;
        printf("Персонаж уже существует. Удалить? [1-да / 0-нет]: ");
        if (read_int_range(0, 1, &ok) != OK || !ok)
            return old_player;
        player_destroy(old_player);
    }
    char name[MAX_FARMER_NAME];
    ask_farmer_name(name, MAX_FARMER_NAME);
    return player_create(name);
}

/* ======================= ПОДМЕНЮ: СНАРЯЖЕНИЕ ======================= */

static void menu_equipment(Player* player)
{
    if (player == NULL) { printf("Сначала создайте персонажа!\n"); return; }

    int choice = -1;
    while (choice != 0) {
        printf("\n=== УПРАВЛЕНИЕ СНАРЯЖЕНИЕМ ===\n");
        player_show_equipment(player);
        printf("[1] Надеть предмет\n");
        printf("[2] Снять предмет\n");
        printf("[0] Назад\n");
        printf("Выбор: ");

        if (read_int_range(0, 2, &choice) != OK) {
            printf("Ошибка ввода.\n");
            continue;
        }

        switch (choice) {
        case 1: {
            int count = player_get_preset_count();
            printf("\n--- ГАРДЕРОБ ---\n");
            for (int i = 0; i < count; i++) {
                const Equipment* e = player_get_preset(i);
                static const char* slot_names[EQUIP_SLOTS] = { "Голова", "Тело", "Обувь" };
                printf("[%d] %s (%s) +%d HP +%d ST\n",
                    i + 1, e->name, slot_names[e->slot],
                    e->bonus_hp, e->bonus_stamina);
            }
            printf("[0] Отмена\n");
            printf("Выберите номер: ");

            int pick;
            if (read_int_range(0, count, &pick) != OK) {
                printf("Ошибка ввода.\n"); break;
            }
            if (pick == 0) break;

            const Equipment* chosen = player_get_preset(pick - 1);
            if (chosen == NULL) { printf("Неверный номер.\n"); break; }

            player_equip_item(player, chosen);
            printf("Надет: %s. max_hp=%d max_stamina=%d\n",
                chosen->name, player->max_hp, player->max_stamina);
            break;
        }
        case 2: {
            int slot_choice;
            printf("Какой слот снять (0-Голова, 1-Тело, 2-Обувь): ");
            if (read_int_range(0, EQUIP_SLOTS - 1, &slot_choice) != OK) {
                printf("Неверный слот!\n"); break;
            }
            if (player_unequip_slot(player, (enum EquipSlot)slot_choice))
                printf("Предмет снят. max_hp=%d max_stamina=%d\n",
                    player->max_hp, player->max_stamina);
            else
                printf("Этот слот пуст.\n");
            break;
        }
        case 0:
        default: break;
        }
    }
}

/* ======================= ПОДМЕНЮ: ВРЕМЯ (4,5,6 → 4) ======================= */

static void menu_time(GameState* game, Player* player)
{
    if (game == NULL) { printf("Нет GameState.\n"); return; }

    int choice = -1;
    while (choice != 0) {
        printf("\n=== УПРАВЛЕНИЕ ВРЕМЕНЕМ ===\n");
        game_show_time(game);
        printf("[1] Посмотреть на часы\n");
        printf("[2] Промотать время (до 02:00)\n");
        printf("[3] Лечь спать (отдых до утра)\n");
        printf("[0] Назад\n");
        printf("Выбор: ");

        if (read_int_range(0, 3, &choice) != OK) {
            printf("Ошибка ввода.\n");
            continue;
        }

        switch (choice) {
        case 1:
            game_show_time(game);
            break;
        case 2: {
            int max_hours;
            if (game->current_hour < NIGHT_HOUR)
                max_hours = NIGHT_HOUR - game->current_hour;
            else
                max_hours = (24 - game->current_hour) + NIGHT_HOUR;

            int hours;
            printf("Сколько часов промотать? (0-%d): ", max_hours);
            if (read_int_range(0, max_hours, &hours) != OK) {
                printf("Ошибка: можно от 0 до %d ч.\n", max_hours);
                break;
            }
            game_skip_time(game, hours);
            break;
        }
        case 3:
            if (player == NULL) { printf("Сначала создайте персонажа!\n"); break; }
            game_sleep(game);
            player->hp = player->max_hp;
            player->stamina = player->max_stamina;
            printf("HP и выносливость восстановлены.\n");
            break;
        case 0:
        default: break;
        }
    }
}

/* ======================= ПОДМЕНЮ: ИНВЕНТАРЬ (7-11 → 5) ======================= */

static void menu_inventory(Player* player)
{
    if (player == NULL) { printf("Сначала создайте персонажа!\n"); return; }

    int choice = -1;
    while (choice != 0) {
        printf("\n=== ИНВЕНТАРЬ ===\n");
        printf("[1] Посмотреть инвентарь\n");
        printf("[2] Положить предмет в слот\n");
        printf("[3] Выбросить предмет\n");
        printf("[4] Проверить соседние ячейки\n");
        printf("[5] Поиск предмета по названию\n");
        printf("[0] Назад\n");
        printf("Выбор: ");

        if (read_int_range(0, 5, &choice) != OK) {
            printf("Ошибка ввода.\n");
            continue;
        }

        switch (choice) {
        case 1: player_print_inventory(player); break;
        case 2: {
            int slot, id;
            printf("Введите номер слота (0-%d): ", MAX_INVENTORY - 1);
            if (read_int_range(0, MAX_INVENTORY - 1, &slot) != OK) {
                printf("Ошибка слота.\n"); break;
            }
            printf("Введите ID предмета (0-%d): ", MAX_ITEMS - 1);
            if (read_int_range(0, MAX_ITEMS - 1, &id) != OK) {
                printf("Ошибка ID.\n"); break;
            }
            player_add_item_to_slot(player, slot, id);
            break;
        }
        case 3: {
            int slot;
            printf("Введите номер слота для очистки (0-%d): ", MAX_INVENTORY - 1);
            if (read_int_range(0, MAX_INVENTORY - 1, &slot) != OK) {
                printf("Ошибка слота.\n"); break;
            }
            player_clear_slot(player, slot);
            break;
        }
        case 4: {
            int id1, id2;
            printf("ID первого предмета (0-%d): ", MAX_ITEMS - 1);
            if (read_int_range(0, MAX_ITEMS - 1, &id1) != OK) { printf("Ошибка\n"); break; }
            printf("ID второго предмета (0-%d): ", MAX_ITEMS - 1);
            if (read_int_range(0, MAX_ITEMS - 1, &id2) != OK) { printf("Ошибка\n"); break; }
            player_check_neighbours(player, id1, id2);
            break;
        }
        case 5: {
            char query[MAX_NAME_LEN];
            printf("Введите название предмета: ");
            if (read_line(query, sizeof(query)) != OK || query[0] == '\0') {
                printf("Ошибка ввода.\n"); break;
            }
            player_search_item(player, query);
            break;
        }
        case 0:
        default: break;
        }
    }
}

/* ======================= ПОДМЕНЮ: ЗАПИСИ (12,13 → 6) ======================= */

static void menu_records(Player* player, const GameState* game)
{
    int choice = -1;
    while (choice != 0) {
        printf("\n=== ЗАПИСИ И ФАЙЛЫ ===\n");
        printf("[1] Записать состояние в дневник (diary.txt)\n");
        printf("[2] Поиск по энциклопедии (input.txt -> output.txt)\n");
        printf("[0] Назад\n");
        printf("Выбор: ");

        if (read_int_range(0, 2, &choice) != OK) {
            printf("Ошибка ввода.\n");
            continue;
        }

        switch (choice) {
        case 1:
            if (player == NULL) { printf("Сначала создайте персонажа!\n"); break; }
            if (game == NULL) break;
            write_diary(player->name, game->current_day, game->current_hour,
                player->inventory, MAX_INVENTORY);
            break;
        case 2:
            process_input_file();
            break;
        case 0:
        default: break;
        }
    }
}

/* ======================= МЕНЮ ======================= */

static void print_header(const Player* player, const GameState* game)
{
    const char* name = (player != NULL) ? player->name : "(нет персонажа)";
    int day = (game != NULL) ? game->current_day : 0;
    int hour = (game != NULL) ? game->current_hour : 0;
    printf("\n========== Ферма: %s | День %d, %02d:00 ==========\n",
        name, day, hour);
}

static void print_menu(void)
{
    printf("[%2d] Создать / пересоздать персонажа\n", MENU_CREATE_PLAYER);
    printf("[%2d] Карточка персонажа\n", MENU_PLAYER_CARD);
    printf("[%2d] Управление снаряжением\n", MENU_EQUIPMENT);
    printf("[%2d] Управление временем\n", MENU_TIME);
    printf("[%2d] Инвентарь\n", MENU_INVENTORY_MENU);
    printf("[%2d] Записи и файлы\n", MENU_RECORDS_MENU);
    printf("[%2d] Отправиться в приключение\n", MENU_ADVENTURE);
    printf("[%2d] Выход\n", MENU_EXIT);
    printf("Выбор: ");
}

/* ======================= MAIN ======================= */

int main(void)
{
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif

    srand((unsigned)time(NULL));
    items_load("items.txt");

    Player* player = NULL;
    GameState* game = game_create();
    if (game == NULL) { printf("Критическая ошибка.\n"); return 1; }

    int choice = -1;
    while (choice != MENU_EXIT) {
        print_header(player, game);
        print_menu();

        if (read_int_range(0, MENU_MAX, &choice) != OK) {
            printf("Ошибка: введите число от 0 до %d.\n", MENU_MAX);
            continue;
        }

        switch (choice) {
        case MENU_CREATE_PLAYER: player = menu_create_player(player); break;
        case MENU_PLAYER_CARD:
            if (player == NULL) { printf("Сначала создайте персонажа!\n"); break; }
            player_print_card(player);
            game_show_time(game);
            break;
        case MENU_EQUIPMENT:     menu_equipment(player);              break;
        case MENU_TIME:          menu_time(game, player);             break;
        case MENU_INVENTORY_MENU:menu_inventory(player);              break;
        case MENU_RECORDS_MENU:  menu_records(player, game);          break;
        case MENU_ADVENTURE:
            if (player == NULL) { printf("Сначала создайте персонажа!\n"); break; }
            adventure_run(game, player);
            break;
        case MENU_EXIT:          printf("Выход из программы.\n");    break;
        default: break;
        }
    }

    if (player != NULL) player_destroy(player);
    if (game != NULL) game_destroy(game);
    return 0;
}