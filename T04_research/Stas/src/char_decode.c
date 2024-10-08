#include <stdio.h>

void encode();  // кодирование
void decode();  // декодирование

int main(int argc, char* argv[]) {
    if (argc == 2) {
        if (argv[1][0] == '0') {
            encode();
            printf("\n");
        } else if (argv[1][0] == '1') {
            decode();
            printf("\n");
        } else {
            printf("n/a");
        }
    } else {
        printf("n/a");
    }

    return 0;
}

void encode() {
    char bukva;
    int cod;
    char space;
    int res = 1;
    int first = 1;

    while (res) {
        // Считываем два символа и пробел
        if (scanf(" %c%c", &bukva, &space) != 2 || (space != '\n' && space != ' ')) {
            printf("n/a");
            res = 0;
        } else {
            cod = bukva;
            if (!first) {
                printf(" ");  // вывод пробела перед следующими кодами
            }
            printf("%X", cod);
            first = 0;
            // Проверяем, является ли следующий символ пробелом (для следующей итерации)
            if (space != ' ') {
                res = 0;
            }
        }
    }
}

void decode() {
    int cod2;
    char space;
    int res = 1;

    while (res) {
        // Считываем два символа и пробел
        if (scanf("%2x%c", &cod2, &space) != 2 || (space != '\n' && space != ' ')) {
            printf("n/a");
            res = 0;
        } else {
            char bukva2 = (char)cod2;
            printf("%c", bukva2);
            // Проверяем, является ли следующий символ пробелом (для следующей итерации)
            if (space != ' ') {
                res = 0;
            }
        }
    }
}