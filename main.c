#include <stdio.h>
#pragma warning(disable : 4996) // чтобы не ворчали на скан ф
#include <windows.h>
#include <locale.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

/* ======================= КОНСТАНТЫ  ======================= */
#define MAX_ITEMS         10
#define MAX_INVENTORY     10
#define MAX_NAME_LEN      64
#define MAX_FARMER_NAME   33
#define MAX_LINE_LEN      256
#define OUTPUT_BUF_LEN    1024
#define MAX_HOURS_INPUT   1000000

/* Коды возврата */
#define OK                1
#define ERR_INPUT         0
#define ERR_NOT_FOUND    -3
#define ERR_MEMORY       -4

#define ITEM_EMPTY        0

#define MENU_MIN          0
#define MENU_MAX          10

/* ======================= КОНСТАНТЫ ПРИКЛЮЧЕНИЯ  ======================= */
#define MAP_MIN_SIZE      5
#define MAP_MAX_SIZE      30
#define MAP_WALL          '#'
#define MAP_FLOOR         '.'
#define MAP_HERO          '@'
#define MAP_TREE          'T'
#define MAP_STONE         'R'
#define MAP_UNKNOWN       '?'
#define MAP_CAMP          '^'     /* кострище / лагерь            */
#define OBSTACLE_MIN_PCT  15
#define OBSTACLE_MAX_PCT  30
#define REVEAL_RADIUS     6       /* радиус «крика»               */
#define REVEAL_STEPS      3       /* шагов видно после крика      */

/* Клавиши */
#define KEY_UP            'W'
#define KEY_UP_LOW        'w'
#define KEY_LEFT          'A'
#define KEY_LEFT_LOW      'a'
#define KEY_DOWN          'S'
#define KEY_DOWN_LOW      's'
#define KEY_RIGHT         'D'
#define KEY_RIGHT_LOW     'd'
#define KEY_REVEAL        'R'
#define KEY_REVEAL_LOW    'r'
#define KEY_QUIT          'Q'
#define KEY_QUIT_LOW      'q'

/* ======================= ГЛОБАЛЬНОЕ СОСТОЯНИЕ ======================= */
static char farmer_name[MAX_FARMER_NAME] = "Фермер";
static char item_names[MAX_ITEMS][MAX_NAME_LEN];
static int  inventory[MAX_INVENTORY] = { 0 };
static int  current_day = 1;
static int  current_hour = 8;

/* ======================= СОСТОЯНИЕ ПРИКЛЮЧЕНИЯ ======================= */
static char** map = NULL;  /* динамическая карта               */
static int    map_h = 0;     /* высота                           */
static int    map_w = 0;     /* ширина                           */
static int    hero_x = 0;     /* столбец героя                    */
static int    hero_y = 0;     /* строка героя                     */
static int    camp_x = 0;     /* столбец кострища                 */
static int    camp_y = 0;     /* строка кострища                  */
static int    reveal_timer = 0;  /* шагов «света» осталось           */
static int    adv_active = 0;  /* идёт ли приключение              */
static int    distance_from_home = 0; /* сколько карт пройдено       */

/* ======================= ЗАЩИТА ВВОДА ======================= */

static int read_line(char* buf, int size)
{
    if (fgets(buf, size, stdin) == NULL)
        return ERR_INPUT;

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
    {
        buf[len - 1] = '\0';
        return OK;
    }

    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
    return ERR_INPUT;
}

static int read_int_range(int min, int max, int* out)
{
    char buf[MAX_LINE_LEN];
    char* end;

    if (read_line(buf, sizeof(buf)) != OK)
        return ERR_INPUT;

    long v = strtol(buf, &end, 10);
    if (end == buf || *end != '\0')
        return ERR_INPUT;
    if (v < min || v > max)
        return ERR_INPUT;

    *out = (int)v;
    return OK;
}

/* ======================= КАТАЛОГ ПРЕДМЕТОВ ======================= */

static void fill_default_items(void)
{
    static const char* defaults[MAX_ITEMS] = {
        "Пусто", "Дерево", "Камень", "Семена",
        "Вода", "Удобрение", "Руда", "Саженцы",
        "Ягоды", "Компостер"
    };
    for (int i = 0; i < MAX_ITEMS; i++)
    {
        strncpy(item_names[i], defaults[i], MAX_NAME_LEN - 1);
        item_names[i][MAX_NAME_LEN - 1] = '\0';
    }
}

static void load_items(const char* filename)
{
    fill_default_items();

    FILE* f = fopen(filename, "r");
    if (f == NULL)
    {
        printf("Внимание: файл '%s' не найден — используются названия по умолчанию.\n",
            filename);
        return;
    }

    char line[MAX_LINE_LEN];
    int loaded = 0;

    while (fgets(line, sizeof(line), f) != NULL)
    {
        int id;
        char name[MAX_NAME_LEN];
        if (sscanf(line, "%d %63[^\n]", &id, name) == 2)
        {
            if (id >= 0 && id < MAX_ITEMS)
            {
                strncpy(item_names[id], name, MAX_NAME_LEN - 1);
                item_names[id][MAX_NAME_LEN - 1] = '\0';
                loaded++;
            }
        }
    }
    fclose(f);
    printf("Загружено названий предметов: %d из %d.\n", loaded, MAX_ITEMS);
}

static const char* item_name(int id)
{
    if (id < 0 || id >= MAX_ITEMS) return "???";
    return item_names[id];
}

/* ======================= ИМЯ ФЕРМЕРА ======================= */

static void ask_farmer_name(void)
{
    char buf[MAX_FARMER_NAME];

    while (1)
    {
        printf("Введите имя фермера (до %d символов): ", MAX_FARMER_NAME - 1);
        if (read_line(buf, sizeof(buf)) == OK && buf[0] != '\0')
        {
            strncpy(farmer_name, buf, MAX_FARMER_NAME - 1);
            farmer_name[MAX_FARMER_NAME - 1] = '\0';
            printf("Привет, %s!\n", farmer_name);
            return;
        }
        printf("Ошибка: имя не может быть пустым и длиннее %d символов.\n",
            MAX_FARMER_NAME - 1);
    }
}

/* ======================= СТАРЫЙ ФУНКЦИОНАЛ (лабы 2-3) ======================= */

static void show_time(void)
{
    printf("Текущее время: День %d, %02d:00\n", current_day, current_hour);
}

static void skip_time(void)
{
    int hours;
    printf("Сколько часов поработать? ");
    if (read_int_range(0, MAX_HOURS_INPUT, &hours) != OK)
    {
        printf("Некорректное число часов.\n");
        return;
    }

    current_hour += hours;
    while (current_hour >= 24)
    {
        current_hour -= 24;
        current_day += 1;
    }
    printf("Прошло %d ч. Теперь: День %d, %02d:00\n",
        hours, current_day, current_hour);
}

static void print_inventory(void)
{
    printf("Инвентарь:\n");
    for (int i = 0; i < MAX_INVENTORY; i++)
    {
        int id = inventory[i];
        if (id == ITEM_EMPTY)
            printf("Слот %d: [0] (Пусто)\n", i);
        else
            printf("Слот %d: [%d] (%s)\n", i, id, item_name(id));
    }
}

static void add_item_to_slot(void)
{
    int slot, id;
    printf("Введите номер слота (0-%d): ", MAX_INVENTORY - 1);
    if (read_int_range(0, MAX_INVENTORY - 1, &slot) != OK)
    {
        printf("Ошибка: номер слота от 0 до %d.\n", MAX_INVENTORY - 1);
        return;
    }
    printf("Введите ID предмета (0-%d): ", MAX_ITEMS - 1);
    if (read_int_range(0, MAX_ITEMS - 1, &id) != OK)
    {
        printf("Ошибка: ID предмета от 0 до %d.\n", MAX_ITEMS - 1);
        return;
    }
    inventory[slot] = id;
    printf("В слот %d положен предмет [%d] (%s).\n",
        slot, id, item_name(id));
}

static void remove_item_from_slot(void)
{
    int slot;
    printf("Введите номер слота для очистки (0-%d): ", MAX_INVENTORY - 1);
    if (read_int_range(0, MAX_INVENTORY - 1, &slot) != OK)
    {
        printf("Ошибка: номер слота от 0 до %d.\n", MAX_INVENTORY - 1);
        return;
    }
    inventory[slot] = ITEM_EMPTY;
    printf("Слот %d очищен.\n", slot);
}

static void check_neighbours(void)
{
    int id1, id2;
    printf("Введите ID первого предмета (0-%d): ", MAX_ITEMS - 1);
    if (read_int_range(0, MAX_ITEMS - 1, &id1) != OK)
    {
        printf("Некорректный ввод.\n"); return;
    }
    printf("Введите ID второго предмета (0-%d): ", MAX_ITEMS - 1);
    if (read_int_range(0, MAX_ITEMS - 1, &id2) != OK)
    {
        printf("Некорректный ввод.\n"); return;
    }

    int found = 0;
    for (int i = 0; i < MAX_INVENTORY - 1; i++)
    {
        int a = inventory[i];
        int b = inventory[i + 1];
        if ((a == id1 && b == id2) || (a == id2 && b == id1))
        {
            printf("Да! Предметы [%d] (%s) и [%d] (%s) лежат рядом в слотах %d и %d.\n",
                id1, item_name(id1), id2, item_name(id2), i, i + 1);
            found = 1;
        }
    }
    if (!found)
        printf("Нет. Предметы [%d] (%s) и [%d] (%s) не лежат рядом.\n",
            id1, item_name(id1), id2, item_name(id2));
}

/* ======================= [7] ПОИСК ПО НАЗВАНИЮ ======================= */

static int find_item_id_by_name(const char* query)
{
    for (int i = 0; i < MAX_ITEMS; i++)
        if (strcmp(item_names[i], query) == 0)
            return i;
    return ERR_NOT_FOUND;
}

static void search_item_by_name(void)
{
    char query[MAX_NAME_LEN];
    printf("Введите название предмета: ");
    if (read_line(query, sizeof(query)) != OK || query[0] == '\0')
    {
        printf("Ошибка ввода названия.\n"); return;
    }

    int id = find_item_id_by_name(query);
    if (id == ERR_NOT_FOUND)
    {
        printf("Предмет '%s' не найден в каталоге.\n", query); return;
    }

    int found = 0;
    for (int i = 0; i < MAX_INVENTORY; i++)
        if (inventory[i] == id)
        {
            printf("Найден в слоте %d: [%d] - %s\n", i, id, item_names[id]);
            found++;
        }
    if (found == 0)
        printf("Предмет '%s' (ID %d) есть в каталоге, но в рюкзаке его нет.\n",
            query, id);
    else
        printf("Всего найдено: %d шт.\n", found);
}

/* ======================= [8] ДНЕВНИК ======================= */

static void write_diary(void)
{
    FILE* f = fopen("diary.txt", "a");
    if (f == NULL)
    {
        printf("Ошибка: не удалось открыть diary.txt для записи.\n"); return;
    }

    fprintf(f, "--Запись дневника--\n");
    fprintf(f, "Фермер: %s\n", farmer_name);
    fprintf(f, "День: %d, Час: %d\n", current_day, current_hour);
    fprintf(f, "Дальность от дома: %d карт\n", distance_from_home);
    fprintf(f, "Инвентарь:\n");

    int empty = 1;
    for (int i = 0; i < MAX_INVENTORY; i++)
    {
        int id = inventory[i];
        fprintf(f, "  Слот %2d: [%d] (%s)\n", i, id, item_name(id));
        if (id != ITEM_EMPTY) empty = 0;
    }
    if (empty) fprintf(f, "  (рюкзак пуст)\n");
    fprintf(f, "===========================\n\n");
    fclose(f);
    printf("Состояние записано в diary.txt.\n");
}

/* ======================= [9] ПОИСКОВИК ======================= */

static unsigned char cp1251_to_lower(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return (unsigned char)(c + 32);
    if (c >= 'А' && c <= 'Я') return (unsigned char)(c + 32);
    if (c == 'Ё')             return (unsigned char)'ё';
    return c;
}

static int find_substr_ci(const char* haystack, const char* needle)
{
    if (*needle == '\0') return 0;
    size_t nlen = strlen(needle);
    for (size_t i = 0; haystack[i] != '\0'; i++)
    {
        size_t j = 0;
        while (j < nlen && haystack[i + j] != '\0' &&
            cp1251_to_lower((unsigned char)haystack[i + j]) ==
            cp1251_to_lower((unsigned char)needle[j]))
            j++;
        if (j == nlen) return (int)i;
    }
    return -1;
}

static int wrap_all_occurrences(const char* src, const char* needle,
    char* dst, size_t dst_size)
{
    size_t nlen = strlen(needle);
    size_t out = 0;

    if (nlen == 0)
    {
        if (strlen(src) + 1 > dst_size) return -1;
        strcpy(dst, src);
        return 0;
    }

    size_t i = 0;
    while (src[i] != '\0')
    {
        if (cp1251_to_lower((unsigned char)src[i]) ==
            cp1251_to_lower((unsigned char)needle[0]))
        {
            size_t j = 0;
            while (j < nlen && src[i + j] != '\0' &&
                cp1251_to_lower((unsigned char)src[i + j]) ==
                cp1251_to_lower((unsigned char)needle[j]))
                j++;
            if (j == nlen)
            {
                if (out + 2 + nlen >= dst_size) return -1;
                dst[out++] = '[';
                for (size_t k = 0; k < nlen; k++) dst[out++] = src[i + k];
                dst[out++] = ']';
                i += nlen;
                continue;
            }
        }
        if (out + 1 >= dst_size) return -1;
        dst[out++] = src[i++];
    }
    dst[out] = '\0';
    return 0;
}

static void process_input_file(void)
{
    char query[MAX_NAME_LEN];
    printf("Введите поисковое слово: ");
    if (read_line(query, sizeof(query)) != OK || query[0] == '\0')
    {
        printf("Ошибка: пустой поисковый запрос.\n"); return;
    }

    FILE* in = fopen("input.txt", "r");
    if (in == NULL)
    {
        printf("Ошибка: файл input.txt не найден.\n"); return;
    }

    FILE* out = fopen("output.txt", "w");
    if (out == NULL)
    {
        printf("Ошибка: не удалось открыть output.txt.\n"); fclose(in); return;
    }

    char line[MAX_LINE_LEN];
    char wrapped[OUTPUT_BUF_LEN];
    int article_no = 0, found = 0;

    printf("\n--- Найденные статьи ---\n");
    while (fgets(line, sizeof(line), in) != NULL)
    {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';
        if (len == 0) continue;
        article_no++;
        if (find_substr_ci(line, query) < 0) continue;
        if (wrap_all_occurrences(line, query, wrapped, sizeof(wrapped)) != 0)
        {
            printf("Предупреждение: статья %d слишком длинная.\n", article_no); continue;
        }
        printf("[Статья %d] %s\n", article_no, wrapped);
        fprintf(out, "[Статья %d] %s\n", article_no, wrapped);
        found++;
    }
    fclose(in);
    fclose(out);

    if (found == 0)
        printf("Совпадений по запросу \"%s\" не найдено.\n", query);
    else
        printf("\nВсего найдено статей: %d (записано в output.txt).\n", found);
}

/* ======================= [10] ПРИКЛЮЧЕНИЕ ======================= */

/* Освобождение карты (сначала строки, потом массив указателей). */
static void free_map(void)
{
    if (map == NULL) return;
    for (int i = 0; i < map_h; i++)
        free(map[i]);
    free(map);
    map = NULL;
    map_h = 0;
    map_w = 0;
}

/* Выделение карты height × width. При ошибке — откат. */
static int alloc_map(int height, int width)
{
    map = (char**)malloc(height * sizeof(char*));
    if (map == NULL) return ERR_MEMORY;

    for (int i = 0; i < height; i++)
    {
        map[i] = (char*)malloc(width * sizeof(char));
        if (map[i] == NULL)
        {
            for (int k = 0; k < i; k++) free(map[k]);
            free(map);
            map = NULL;
            return ERR_MEMORY;
        }
    }
    map_h = height;
    map_w = width;
    return OK;
}

/* Поставить героя на случайную свободную клетку внутри. */
static void place_hero(void)
{
    if (map_h < 3 || map_w < 3) { hero_y = hero_x = 1; return; }

    int attempts = 0;
    do {
        hero_y = 1 + rand() % (map_h - 2);
        hero_x = 1 + rand() % (map_w - 2);
        attempts++;
    } while (map[hero_y][hero_x] != MAP_FLOOR && attempts < 1000);
}

/* Поставить кострище на случайную свободную клетку, отличную от героя. */
static void place_camp(void)
{
    if (map_h < 3 || map_w < 3) { camp_y = camp_x = 1; return; }

    int attempts = 0;
    do {
        camp_y = 1 + rand() % (map_h - 2);
        camp_x = 1 + rand() % (map_w - 2);
        attempts++;
    } while ((map[camp_y][camp_x] != MAP_FLOOR ||
        (camp_y == hero_y && camp_x == hero_x)) && attempts < 1000);

    map[camp_y][camp_x] = MAP_CAMP;
}

/* Генерация карты: без сплошных границ — стены только по краям,
   но каждый край с вероятностью ~50% имеет проход. */
static int generate_map(void)
{
    free_map();

    int h = MAP_MIN_SIZE + rand() % (MAP_MAX_SIZE - MAP_MIN_SIZE + 1);
    int w = MAP_MIN_SIZE + rand() % (MAP_MAX_SIZE - MAP_MIN_SIZE + 1);

    if (alloc_map(h, w) != OK)
    {
        printf("Ошибка: не удалось выделить память под карту %dx%d.\n", h, w);
        return ERR_MEMORY;
    }

    /* Заполняем всё полом, затем рисуем стены по краям */
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            map[y][x] = MAP_FLOOR;

    /* Верхний и нижний края — с проходами */
    for (int x = 0; x < w; x++)
    {
        if (rand() % 2 == 0) map[0][x] = MAP_WALL;
        if (rand() % 2 == 0) map[h - 1][x] = MAP_WALL;
    }
    /* Левый и правый края — с проходами */
    for (int y = 0; y < h; y++)
    {
        if (rand() % 2 == 0) map[y][0] = MAP_WALL;
        if (rand() % 2 == 0) map[y][w - 1] = MAP_WALL;
    }
    /* Углы — всегда стены (чтобы не было «дырявых» углов) */
    map[0][0] = MAP_WALL;
    map[0][w - 1] = MAP_WALL;
    map[h - 1][0] = MAP_WALL;
    map[h - 1][w - 1] = MAP_WALL;

    /* Внутренние препятствия (только внутри, не на краях) */
    for (int y = 1; y < h - 1; y++)
    {
        for (int x = 1; x < w - 1; x++)
        {
            int pct = OBSTACLE_MIN_PCT +
                rand() % (OBSTACLE_MAX_PCT - OBSTACLE_MIN_PCT + 1);
            int r = rand() % 100;
            if (r < pct / 2)      map[y][x] = MAP_TREE;
            else if (r < pct)     map[y][x] = MAP_STONE;
            else                  map[y][x] = MAP_FLOOR;
        }
    }

    /* Герой и кострище */
    place_hero();
    place_camp();

    reveal_timer = 0;
    return OK;
}

/* Можно ли пройти в клетку (x, y)? Здесь границы НЕ блокируют —
   выход за край означает переход на новую карту. */
static int is_walkable(int x, int y)
{
    /* выход за пределы массива — разрешаем (это переход) */
    if (x < 0 || x >= map_w || y < 0 || y >= map_h) return 1;

    char c = map[y][x];
    return c != MAP_WALL && c != MAP_TREE && c != MAP_STONE;
}

/* Отрисовка карты с учётом «тьмы» (вариант 6). */
static void draw_map(void)
{
    printf("\n");
    for (int y = 0; y < map_h; y++)
    {
        for (int x = 0; x < map_w; x++)
        {
            char cell = map[y][x];
            int in_reveal = 0;

            if (reveal_timer > 0)
            {
                int dx = x - hero_x;
                int dy = y - hero_y;
                if (dx * dx + dy * dy <= REVEAL_RADIUS * REVEAL_RADIUS)
                    in_reveal = 1;
            }

            if (in_reveal)
            {
                /* В радиусе видно как есть; герой выделяется */
                if (cell == MAP_HERO) putchar(MAP_HERO);
                else                  putchar(cell);
            }
            else
            {
                /* Вне радиуса — тьма, но герой всегда виден */
                if (cell == MAP_HERO) putchar(MAP_HERO);
                else                  putchar(MAP_UNKNOWN);
            }
        }
        putchar('\n');
    }

    if (reveal_timer > 0)
        printf("Свет: ещё %d шаг(а) видно.\n", reveal_timer);
    else
        printf("Темно. Нажмите R, чтобы крикнуть.\n");

    printf("Герой: (%d, %d) | Дальность от дома: %d карт\n",
        hero_x, hero_y, distance_from_home);
    printf("W/A/S/D — шаг, R — крик, Q — домой (только на '^').\n");
}

/* Шаг героя. Если шаг уводит за край — переход на новую карту. */
static void try_move(int dx, int dy)
{
    int nx = hero_x + dx;
    int ny = hero_y + dy;

    /* --- Переход на новую карту: шаг за пределы массива --- */
    if (nx < 0 || nx >= map_w || ny < 0 || ny >= map_h)
    {
        printf("Вы вышли за край леса... Генерируется новый участок.\n");

        free_map();                          /* освобождаем старую карту */
        distance_from_home++;                /* +1 к дальности           */

        if (generate_map() != OK)
        {
            printf("Ошибка генерации новой карты. Возврат домой.\n");
            adv_active = 0;
            return;
        }
        return;                              /* герой уже на новой карте */
    }

    /* --- Обычный шаг --- */
    if (!is_walkable(nx, ny))
    {
        printf("Туда нельзя — препятствие или стена.\n");
        return;
    }

    char target = map[ny][nx];

    map[hero_y][hero_x] = MAP_FLOOR;         /* стереть старого героя    */
    hero_x = nx;
    hero_y = ny;

    if (target == MAP_CAMP)
        printf("Вы у костра. Можно отдохнуть (Q).\n");

    map[hero_y][hero_x] = MAP_HERO;          /* поставить нового героя   */

    if (reveal_timer > 0) reveal_timer--;
}

/* Главный цикл приключения. */
static void adventure_loop(void)
{
    printf("\n=== НОЧНОЙ ЛЕС ===\n");
    printf("Карта %dx%d. Вокруг тьма. Нажмите R, чтобы крикнуть.\n",
        map_w, map_h);

    adv_active = 1;
    char buf[MAX_LINE_LEN];

    while (adv_active)
    {
        draw_map();
        printf("Ваш ход: ");

        if (read_line(buf, sizeof(buf)) != OK || buf[0] == '\0')
        {
            printf("Ошибка ввода. Введите W/A/S/D/R/Q.\n");
            continue;
        }

        char c = buf[0];

        if (c == KEY_UP || c == KEY_UP_LOW)              try_move(0, -1);
        else if (c == KEY_LEFT || c == KEY_LEFT_LOW)     try_move(-1, 0);
        else if (c == KEY_DOWN || c == KEY_DOWN_LOW)     try_move(0, 1);
        else if (c == KEY_RIGHT || c == KEY_RIGHT_LOW)   try_move(1, 0);
        else if (c == KEY_REVEAL || c == KEY_REVEAL_LOW)
        {
            reveal_timer = REVEAL_STEPS;
            printf("Вы крикнули! Препятствия в радиусе %d раскрыты на %d шага.\n",
                REVEAL_RADIUS, REVEAL_STEPS);
        }
        else if (c == KEY_QUIT || c == KEY_QUIT_LOW)
        {
            /* Q работает только если герой стоит на клетке костра */
            if (map[hero_y][hero_x] == MAP_CAMP)
            {
                adv_active = 0;
                printf("Вы отдохнули у костра и вернулись домой.\n");
            }
            else
            {
                printf("Здесь негде отдохнуть. Найдите костёр '^'.\n");
            }
        }
        else
        {
            printf("Неизвестная команда '%c'. Используйте W/A/S/D/R/Q.\n", c);
        }
    }
}

/* Пункт [10]: запуск приключения с гарантированной очисткой. */
static void start_adventure(void)
{
    distance_from_home = 0;                  /* сброс счётчика           */

    if (generate_map() != OK)
        return;

    adventure_loop();

    free_map();
    printf("Память карты освобождена. Пройдено карт: %d.\n",
        distance_from_home);
}

/* ======================= МЕНЮ И MAIN ======================= */

static void print_header(void)
{
    printf("\n========== Ферма: %s | День %d, %02d:00 ==========\n",
        farmer_name, current_day, current_hour);
}

static void print_menu(void)
{
    printf("[0] Выход\n");
    printf("[1] Посмотреть на часы\n");
    printf("[2] Промотать время (Поработать)\n");
    printf("[3] Посмотреть инвентарь\n");
    printf("[4] Положить предмет в слот\n");
    printf("[5] Выбросить предмет\n");
    printf("[6] Соседние ячейки\n");
    printf("[7] Поиск предмета по названию\n");
    printf("[8] Записать состояние в дневник\n");
    printf("[9] Поиск по энциклопедии\n");
    printf("[10] Отправиться в приключение\n");
    printf("Выбор: ");
}

int main(void)
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "Russian");

    srand((unsigned)time(NULL));

    for (int i = 0; i < MAX_INVENTORY; i++) inventory[i] = ITEM_EMPTY;
    if (MAX_INVENTORY >= 7)
    {
        inventory[0] = 1; inventory[1] = 2; inventory[2] = 3;
        inventory[3] = 4; inventory[4] = 7; inventory[5] = 8; inventory[6] = 9;
    }

    load_items("items.txt");
    ask_farmer_name();

    int choice;
    int running = 1;

    while (running)
    {
        print_header();
        print_menu();

        if (read_int_range(MENU_MIN, MENU_MAX, &choice) != OK)
        {
            printf("Ошибка: введите число от %d до %d.\n",
                MENU_MIN, MENU_MAX);
            continue;
        }

        switch (choice)
        {
        case 0:  running = 0;                 break;
        case 1:  show_time();                 break;
        case 2:  skip_time();                 break;
        case 3:  print_inventory();           break;
        case 4:  add_item_to_slot();          break;
        case 5:  remove_item_from_slot();     break;
        case 6:  check_neighbours();          break;
        case 7:  search_item_by_name();       break;
        case 8:  write_diary();               break;
        case 9:  process_input_file();        break;
        case 10: start_adventure();           break;
        default: break;
        }
    }

    free_map();
    printf("Выход из программы. До встречи, %s!\n", farmer_name);
    return 0;
}