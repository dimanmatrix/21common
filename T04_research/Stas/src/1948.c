#include <stdio.h>

// Функция для поиска остатка от деления
int ostdel(int x, int y);
// Функция поиска делителя
int delitel();
// Функция для определения условия, является ли делитель простым
int is_simple();

int main() {
    float a;
    if (scanf("%f", &a)) {
        int x = (int)a;
        if (x < 0) {
            x = x * (-1);
            a = a * (-1);
        }

        if (a == x) {
            if (is_simple(x) || x == 0 || x == 1) {
                printf("%d", x);
                return 0;
            }
            int maxdel = delitel(x);
            while (is_simple(maxdel) != 1) maxdel = delitel(maxdel);  // делим делитель снова
            printf("%d\n", maxdel);
        } else
            printf("n/a\n");
    } else
        printf("n/a\n");
    return 0;
}

int ostdel(int x, int y) {
    for (int i = x; i >= 0; i = i - y) x = x - y;
    return x + y;
}

int delitel(int x) {
    int i = x - 1;
    while (ostdel(x, i) != 0 && i < x && i > 1) i--;
    return i;
}

int is_simple(int x) {
    if (x > 1) {
        for (int i = 2; i < x; i++)
            if (ostdel(x, i) == 0)
                return 0;  // если число делится без остатка на i - возвращаем 0  / выход (число не простое)
        return 1;          // если не разделилось - возвращаем 1 - простой
    } else                 // иначе возвращаем 0
        return 0;
}
