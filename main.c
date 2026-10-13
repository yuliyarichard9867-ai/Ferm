#include <stdio.h>
#pragma warning(disable : 4996) //Чтобы не ворчали на скан ф
#include <windows.h>
#include <locale.h>
static const char* item_name(int id) {
    static const char* names[10] = {
        "Пусто",        /* 0 */
        "Дерево",       /* 1 */
        "Камень",       /* 2 */
        "Семена",       /* 3 */
        "Вода",         /* 4 */
        "Удобрение",    /* 5 */
        "Руда",         /* 6 */
        "Саженцы",      /* 7 */
        "Ягоды",        /* 8 */
        "Компостер"     /* 9 */
    };

    if (id < 0 || id > 9) return "???";
    return names[id];
}

int main(void) {
   
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001); //От косяков в виде кривых символов и прочего
    setlocale(LC_ALL, "");

  
    int current_day = 1;
    int current_hour = 8;
    int inventory[10] = { 0 }; 

  
    inventory[0] = 1;  /* Дерево    */
    inventory[1] = 2;  /* Камень    */
    inventory[2] = 3;  /* Семена    */
    inventory[3] = 4;  /* Вода      */
    inventory[4] = 7;  /* Саженцы   */
    inventory[5] = 8;  /* Ягоды     */
    inventory[6] = 9;  /* Компостер */

    int choice;

    do {
        printf("\n===== МЕНЮ =====\n");  //Добавлено меню
        printf("[0] Выход\n");
        printf("[1] Посмотреть на часы\n");
        printf("[2] Промотать время (Поработать)\n");
        printf("[3] Посмотреть инвентарь\n");
        printf("[4] Положить предмет в слот\n");
        printf("[5] Выбросить предмет\n");
        printf("[6] Соседние ячейки\n");
        printf("Выбор: ");

        if (scanf("%d", &choice) != 1) {
            printf("Некорректный ввод.\n");
            while (getchar() != '\n');
            continue;
        }

        switch (choice) {

        case 0:
            printf("Выход из программы. До встречи!\n");
            break;

        case 1:
            printf("Текущее время: День %d, %02d:00\n",
                current_day, current_hour);
            break;

        case 2: {
            int hours;
            printf("Сколько часов поработать? ");        //Добавлено время
            if (scanf("%d", &hours) != 1 || hours < 0) {
                printf("Некорректное число часов.\n");
                while (getchar() != '\n');
                break;
            }

            current_hour += hours;
            while (current_hour >= 24) {
                current_hour -= 24;
                current_day += 1;
            }

            printf("Прошло %d ч. Теперь: День %d, %02d:00\n",
                hours, current_day, current_hour);
            break;
        }

        case 3:
            printf("Инвентарь:\n");
            for (int i = 0; i < 10; i++) {
                int id = inventory[i];
                if (id == 0) {
                    printf("Слот %d: [0] (Пусто)\n", i);
                }
                else {
                    printf("Слот %d: [%d] (%s)\n", i, id, item_name(id));
                }
            }
            break;

        case 4: {
            int slot, id;
            printf("Введите номер слота (0-9): ");   //Кладем предметы в слот
            if (scanf("%d", &slot) != 1) {
                printf("Некорректный номер.\n");
                while (getchar() != '\n');
                break;
            }
            printf("Введите ID предмета (0-9): ");
            if (scanf("%d", &id) != 1) {
                printf("Некорректный ID.\n");
                while (getchar() != '\n');
                break;
            }

            if (slot < 0 || slot > 9) {
                printf("Ошибка: номер слота должен быть от 0 до 9.\n");
                break;
            }
            if (id < 0 || id > 9) {
                printf("Ошибка: ID предмета должен быть от 0 до 9.\n");
                break;
            }

            inventory[slot] = id;
            printf("В слот %d положен предмет [%d] (%s).\n",
                slot, id, item_name(id));
            break;
        }

        case 5: {
            int slot;
            printf("Введите номер слота для очистки (0-9): ");  
            if (scanf("%d", &slot) != 1) {
                printf("Некорректный номер.\n");
                while (getchar() != '\n');
                break;
            }

            if (slot < 0 || slot > 9) {
                printf("Ошибка: номер слота должен быть от 0 до 9.\n"); //Проверка диапазона
                break;
            }

            inventory[slot] = 0;
            printf("Слот %d очищен.\n", slot);
            break;
        }

        case 6: {
            int id1, id2;
            printf("Введите ID первого предмета (0-9): "); 
            if (scanf("%d", &id1) != 1) {
                printf("Некорректный ввод.\n");
                while (getchar() != '\n');
                break;
            }
            printf("Введите ID второго предмета (0-9): ");
            if (scanf("%d", &id2) != 1) {
                printf("Некорректный ввод.\n");
                while (getchar() != '\n');
                break;
            }

            if (id1 < 0 || id1 > 9 || id2 < 0 || id2 > 9) {
                printf("Ошибка: ID должны быть от 0 до 9.\n"); //Проверяем чтобы были цифры
                break;
            }

           
             int found = 0;                           //Добавлен поиск предметов 

            for (int i = 0; i < 9; i++) {
                int a = inventory[i];
                int b = inventory[i + 1];

              
                if ((a == id1 && b == id2) || (a == id2 && b == id1)) {
                    printf("Да! Предметы [%d] (%s) и [%d] (%s) лежат "
                        "рядом в слотах %d и %d.\n",
                        id1, item_name(id1),
                        id2, item_name(id2),
                        i, i + 1);
                    found = 1;
                }
            }

            if (!found) {
                printf("Нет. Предметы [%d] (%s) и [%d] (%s) "
                    "не лежат рядом.\n",
                    id1, item_name(id1),
                    id2, item_name(id2));
            }
            break;
        }

        default:
            printf("Нет такого пункта меню.\n");
            break;
        }

    } while (choice != 0);  
    return 0;               
}