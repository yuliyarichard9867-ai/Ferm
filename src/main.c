#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#define MENU_SHOW_TIME        4
#define MENU_SKIP_TIME        5
#define MENU_INVENTORY        6
#define MENU_ADD_ITEM         7
#define MENU_CLEAR_SLOT       8
#define MENU_NEIGHBOURS       9
#define MENU_SEARCH_ITEM      10
#define MENU_DIARY            11
#define MENU_ENCYCLOPEDIA     12
#define MENU_MAX              12

/* ======================= ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ======================= */

/* Спрашивает имя фермера с защитой от пустого ввода. */
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

/* Создание / пересоздание персонажа с подтверждением удаления старого. */
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

/* Подменю «Управление снаряжением» */  //Из варианта
static void menu_equipment(Player* player)
{
    if (player == NULL) {
        printf("Сначала создайте персонажа!\n");
        return;
    }

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
            Equipment e;
            int slot_choice;

            printf("Название предмета: ");
            if (read_line(e.name, MAX_NAME_LEN) != OK || e.name[0] == '\0') {
                printf("Ошибка ввода названия.\n");
                break;
            }

            printf("Слот (0-Голова, 1-Тело, 2-Обувь): ");
            if (read_int_range(0, EQUIP_SLOTS - 1, &slot_choice) != OK) {
                printf("Неверный слот!\n");
                break;
            }
            e.slot = (enum EquipSlot)slot_choice;

            printf("Бонус к HP: ");
            if (read_int_range(0, 1000, &e.bonus_hp) != OK) {
                printf("Ошибка ввода бонуса HP.\n");
                break;
            }

            printf("Бонус к выносливости: ");
            if (read_int_range(0, 1000, &e.bonus_stamina) != OK) {
                printf("Ошибка ввода бонуса ST.\n");
                break;
            }

            player_equip_item(player, &e);
            printf("Предмет надет. Новые характеристики:\n");
            printf("  max_hp=%d  max_stamina=%d\n",
                player->max_hp, player->max_stamina);
            break;
        }
        case 2: {
            int slot_choice;
            printf("Какой слот снять (0-Голова, 1-Тело, 2-Обувь): ");
            if (read_int_range(0, EQUIP_SLOTS - 1, &slot_choice) != OK) {
                printf("Неверный слот!\n");
                break;
            }
            if (player_unequip_slot(player, (enum EquipSlot)slot_choice))
                printf("Предмет снят. max_hp=%d max_stamina=%d\n",
                    player->max_hp, player->max_stamina);
            else
                printf("Этот слот пуст.\n");
            break;
        }
        case 0:
        default:
            break;
        }
    }
}

/* Добавление предмета в слот инвентаря. */
static void menu_add_item(Player* player)
{
    if (player == NULL) { printf("Нет персонажа.\n"); return; }

    int slot, id;
    printf("Введите номер слота (0-%d): ", MAX_INVENTORY - 1);
    if (read_int_range(0, MAX_INVENTORY - 1, &slot) != OK) {
        printf("Ошибка: номер слота от 0 до %d.\n", MAX_INVENTORY - 1);
        return;
    }
    printf("Введите ID предмета (0-%d): ", MAX_ITEMS - 1);
    if (read_int_range(0, MAX_ITEMS - 1, &id) != OK) {
        printf("Ошибка: ID предмета от 0 до %d.\n", MAX_ITEMS - 1);
        return;
    }
    player_add_item_to_slot(player, slot, id);
}

/* Очистка слота инвентаря. */
static void menu_clear_slot(Player* player)
{
    if (player == NULL) { printf("Нет персонажа.\n"); return; }

    int slot;
    printf("Введите номер слота для очистки (0-%d): ", MAX_INVENTORY - 1);
    if (read_int_range(0, MAX_INVENTORY - 1, &slot) != OK) {
        printf("Ошибка: номер слота от 0 до %d.\n", MAX_INVENTORY - 1);
        return;
    }
    player_clear_slot(player, slot);
}

/* Проверка соседних ячеек. */
static void menu_neighbours(Player* player)
{
    if (player == NULL) { printf("Нет персонажа.\n"); return; }

    int id1, id2;
    printf("Введите ID первого предмета (0-%d): ", MAX_ITEMS - 1);
    if (read_int_range(0, MAX_ITEMS - 1, &id1) != OK) {
        printf("Некорректный ввод.\n");
        return;
    }
    printf("Введите ID второго предмета (0-%d): ", MAX_ITEMS - 1);
    if (read_int_range(0, MAX_ITEMS - 1, &id2) != OK) {
        printf("Некорректный ввод.\n");
        return;
    }
    player_check_neighbours(player, id1, id2);
}

/* Поиск предмета по названию. */
static void menu_search_item(Player* player)
{
    if (player == NULL) { printf("Нет персонажа.\n"); return; }

    char query[MAX_NAME_LEN];
    printf("Введите название предмета: ");
    if (read_line(query, sizeof(query)) != OK || query[0] == '\0') {
        printf("Ошибка ввода названия.\n");
        return;
    }
    player_search_item(player, query);
}

/* Запись дневника — параметры вместо глобалок. */
static void menu_diary(Player* player, const GameState* game)
{
    if (player == NULL) { printf("Нет персонажа.\n"); return; }
    if (game == NULL) { printf("Нет GameState.\n"); return; }

    write_diary(player->name, game->current_day, game->current_hour,
        player->inventory, MAX_INVENTORY);
}

/* ======================= ВЫВОД ШАПКИ И МЕНЮ ======================= */

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
    printf("[%2d] Посмотреть на часы\n", MENU_SHOW_TIME);
    printf("[%2d] Промотать время\n", MENU_SKIP_TIME);
    printf("[%2d] Посмотреть инвентарь\n", MENU_INVENTORY);
    printf("[%2d] Положить предмет в слот\n", MENU_ADD_ITEM);
    printf("[%2d] Выбросить предмет\n", MENU_CLEAR_SLOT);
    printf("[%2d] Соседние ячейки\n", MENU_NEIGHBOURS);
    printf("[%2d] Поиск предмета по названию\n", MENU_SEARCH_ITEM);
    printf("[%2d] Записать состояние в дневник\n", MENU_DIARY);
    printf("[%2d] Поиск по энциклопедии\n", MENU_ENCYCLOPEDIA);
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

    items_load("items.txt");

    Player* player = NULL;
    GameState* game = game_create();

    if (game == NULL) {
        printf("Критическая ошибка: не удалось создать GameState.\n");
        return 1;
    }

    int choice = -1;
    while (choice != MENU_EXIT) {
        print_header(player, game);
        print_menu();

        if (read_int_range(0, MENU_MAX, &choice) != OK) {
            printf("Ошибка: введите число от 0 до %d.\n", MENU_MAX);
            continue;
        }

        switch (choice) {
        case MENU_CREATE_PLAYER:
            player = menu_create_player(player);
            break;

        case MENU_PLAYER_CARD:
            if (player == NULL) { printf("Сначала создайте персонажа!\n"); break; }
            player_print_card(player);
            if (game) game_show_time(game);
            break;

        case MENU_EQUIPMENT:
            menu_equipment(player);
            break;

        case MENU_SHOW_TIME:
            if (game) game_show_time(game);
            break;

        case MENU_SKIP_TIME: {
            if (game == NULL) break;
            int hours;
            printf("Сколько часов промотать? ");
            if (read_int_range(0, 1000000, &hours) != OK) {
                printf("Некорректное число часов.\n");
                break;
            }
            game_skip_time(game, hours);
            break;
        }

        case MENU_INVENTORY:
            if (player == NULL) { printf("Сначала создайте персонажа!\n"); break; }
            player_print_inventory(player);
            break;

        case MENU_ADD_ITEM:
            menu_add_item(player);
            break;

        case MENU_CLEAR_SLOT:
            menu_clear_slot(player);
            break;

        case MENU_NEIGHBOURS:
            menu_neighbours(player);
            break;

        case MENU_SEARCH_ITEM:
            menu_search_item(player);
            break;

        case MENU_DIARY:
            menu_diary(player, game);
            break;

        case MENU_ENCYCLOPEDIA:
            process_input_file();
            break;

        case MENU_EXIT:
            printf("Выход из программы.\n");
            break;

        default:
            break;
        }
    }

    /* Очистка динамической памяти при выходе */
    if (player != NULL) player_destroy(player);
    if (game != NULL) game_destroy(game);

    return 0;
}