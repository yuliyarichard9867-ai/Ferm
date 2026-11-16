#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "player.h"

/* ======================= ГЛОБАЛЬНЫЙ КАТАЛОГ ПРЕДМЕТОВ =======================
 * Это НЕ данные игрока, а статический справочник (аналог таблицы БД).
 * Изменяемое состояние игры (player, day, hour) — без глобалок.
 */
static char item_names[MAX_ITEMS][MAX_NAME_LEN];

/* ======================= ЗАЩИТА ВВОДА ======================= */

int read_line(char *buf, int size)
{
    if (fgets(buf, size, stdin) == NULL)
        return ERR_INPUT;

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
        return OK;
    }

    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
    return ERR_INPUT;
}

int read_int_range(int min, int max, int *out)
{
    char buf[MAX_LINE_LEN];
    char *end;

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
    static const char *defaults[MAX_ITEMS] = {
        "Пусто", "Дерево", "Камень", "Семена",
        "Вода", "Удобрение", "Руда", "Саженцы",
        "Ягоды", "Компостер"
    };
    for (int i = 0; i < MAX_ITEMS; i++) {
        strncpy(item_names[i], defaults[i], MAX_NAME_LEN - 1);
        item_names[i][MAX_NAME_LEN - 1] = '\0';
    }
}

void items_load(const char *filename)
{
    fill_default_items();

    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        printf("Внимание: файл '%s' не найден — используются названия "
               "по умолчанию.\n", filename);
        return;
    }

    char line[MAX_LINE_LEN];
    int loaded = 0;

    while (fgets(line, sizeof(line), f) != NULL) {
        int id;
        char name[MAX_NAME_LEN];

        if (sscanf(line, "%d %63[^\n]", &id, name) == 2) {
            if (id >= 0 && id < MAX_ITEMS) {
                strncpy(item_names[id], name, MAX_NAME_LEN - 1);
                item_names[id][MAX_NAME_LEN - 1] = '\0';
                loaded++;
            }
        }
    }
    fclose(f);
    printf("Загружено названий предметов: %d из %d.\n", loaded, MAX_ITEMS);
}

const char *item_name(int id)
{
    if (id < 0 || id >= MAX_ITEMS) return "???";
    return item_names[id];
}

int find_item_id_by_name(const char *query)
{
    for (int i = 0; i < MAX_ITEMS; i++)
        if (strcmp(item_names[i], query) == 0)
            return i;
    return ERR_NOT_FOUND;
}

/* ======================= ДНЕВНИК ======================= */

void write_diary(const char *farmer_name, int day, int hour,
                 const int *inventory, int inv_size)
{
    FILE *f = fopen("diary.txt", "a");
    if (f == NULL) {
        printf("Ошибка: не удалось открыть diary.txt для записи.\n");
        return;
    }

    fprintf(f, "===== Запись дневника =====\n");
    fprintf(f, "Фермер: %s\n", farmer_name);
    fprintf(f, "День: %d, Час: %d\n", day, hour);
    fprintf(f, "Инвентарь:\n");

    int empty = 1;
    for (int i = 0; i < inv_size; i++) {
        int id = inventory[i];
        fprintf(f, "  Слот %2d: [%d] (%s)\n", i, id, item_name(id));
        if (id != ITEM_EMPTY) empty = 0;
    }
    if (empty)
        fprintf(f, "  (рюкзак пуст)\n");

    fprintf(f, "===========================\n\n");
    fclose(f);
    printf("Состояние записано в diary.txt.\n");
}

/* ======================= [9] ПОИСКОВИК ПО ЭНЦИКЛОПЕДИИ ======================= */

static unsigned char cp1251_to_lower(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return (unsigned char)(c + 32);
    if (c >= 0xC0 && c <= 0xDF) return (unsigned char)(c + 32);
    if (c == 0xA8) return 0xB8;
    return c;
}

static int find_substr_ci(const char *haystack, const char *needle)
{
    if (*needle == '\0') return 0;
    size_t nlen = strlen(needle);

    for (size_t i = 0; haystack[i] != '\0'; i++) {
        size_t j = 0;
        while (j < nlen && haystack[i + j] != '\0' &&
               cp1251_to_lower((unsigned char)haystack[i + j]) ==
               cp1251_to_lower((unsigned char)needle[j]))
            j++;
        if (j == nlen) return (int)i;
    }
    return -1;
}

static int wrap_all_occurrences(const char *src, const char *needle,
                                char *dst, size_t dst_size)
{
    size_t nlen = strlen(needle);
    size_t out  = 0;

    if (nlen == 0) {
        if (strlen(src) + 1 > dst_size) return -1;
        strcpy(dst, src);
        return 0;
    }

    size_t i = 0;
    while (src[i] != '\0') {
        if (cp1251_to_lower((unsigned char)src[i]) ==
            cp1251_to_lower((unsigned char)needle[0])) {
            size_t j = 0;
            while (j < nlen && src[i + j] != '\0' &&
                   cp1251_to_lower((unsigned char)src[i + j]) ==
                   cp1251_to_lower((unsigned char)needle[j]))
                j++;

            if (j == nlen) {
                if (out + 2 + nlen >= dst_size) return -1;
                dst[out++] = '[';
                for (size_t k = 0; k < nlen; k++)
                    dst[out++] = src[i + k];
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

void process_input_file(void)
{
    char query[MAX_NAME_LEN];

    printf("Введите поисковое слово: ");
    if (read_line(query, sizeof(query)) != OK || query[0] == '\0') {
        printf("Ошибка: пустой поисковый запрос.\n");
        return;
    }

    FILE *in = fopen("input.txt", "r");
    if (in == NULL) {
        printf("Ошибка: файл input.txt не найден.\n");
        return;
    }

    FILE *out = fopen("output.txt", "w");
    if (out == NULL) {
        printf("Ошибка: не удалось открыть output.txt для записи.\n");
        fclose(in);
        return;
    }

    char line[MAX_LINE_LEN];
    char wrapped[OUTPUT_BUF_LEN];
    int  article_no = 0;
    int  found      = 0;

    printf("\n--- Найденные статьи ---\n");

    while (fgets(line, sizeof(line), in) != NULL) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';
        if (len == 0) continue;

        article_no++;

        if (find_substr_ci(line, query) < 0)
            continue;

        if (wrap_all_occurrences(line, query, wrapped, sizeof(wrapped)) != 0) {
            printf("Предупреждение: статья %d слишком длинная, пропущена.\n",
                   article_no);
            continue;
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