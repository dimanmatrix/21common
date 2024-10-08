#include <stdio.h>

void encode(char c);
void decode(char high, char low);
int char_to_int(char c);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("n/a");
        return 1;
    }

    // читаем буквы
    if (*argv[1] == '0') {
        char c1, c2;
        int result;
        int first = 1;

        while (1) {
            result = scanf("%c%c", &c1, &c2);

            if (c1 == '\n') {
                break;
            }

            // Если только один символ был прочитан, это должен быть последний символ
            if (result == 1) {
                if (!first) {
                    putchar(' ');
                }
                encode(c1);
                break;
            }

            // если два символа считано
            else {
                if (c2 != ' ' && c2 != '\n') {
                    if (!first) {
                        printf(" ");
                    }
                    printf("n/a");
                    return 1;
                }

                if (!first) {
                    putchar(' ');
                }
                encode(c1);
                first = 0;

                if (c2 == '\n') {
                    break;
                }
            }
        }
        // читаем HEX
    } else if (*argv[1] == '1') {
        char first, second, space;
        int result;

        while (1) {
            result = scanf("%c%c%c", &first, &second, &space);

            if (first == '\n') {
                break;
            }

            if (result < 3 || (space != ' ' && space != '\n')) {
                printf("n/a");
                return 1;
            }

            decode(first, second);

            if (space == '\n') {
                break;
            }
            putchar(' ');
        }
    } else {
        printf("n/a");
        return 1;
    }
    return 0;
}

void encode(char c) {
    if (c == ' ') {
        putchar(' ');
    } else {
        /*
        %X выводит число в шестнадцатеричном формате.
        0 заполняет ведущими нулями, если число меньше двух цифр.
        2 указывает, что нужно вывести ровно две цифры.
        */
        printf("%02X", c);
    }
}

void decode(char first, char second) {
    int h = char_to_int(first);
    int l = char_to_int(second);
    if (h == -1 || l == -1) {
        printf("n/a");
        return;
    }

    printf("%c", h * 16 + l);
}

int char_to_int(char c) {
    // вычитаем код символа '0', чтобы получить цифру
    if (c >= '0' && c <= '9') return c - '0';
    // вычитаем 'A' и добавляем 10 - преобразуем буквы в их числовые значения
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}