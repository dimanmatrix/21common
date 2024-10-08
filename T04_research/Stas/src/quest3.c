#include <stdio.h>

int fibo(int n);

int main() {
    int i;
    char c;

    if (scanf("%d%c", &i, &c) != 2 || c != '\n') {
        printf("n/a");
        return 1;
    }
    printf("%d", fib(i));
    return 0;
}

int fib(int n) {
    if (n <= 0) {
        return 0;  // Если n равно 0, возвращаем 0
    } else if (n == 1) {
        return 1;  // Если n равно 1, возвращаем 1
    } else {
        return fib(n - 1) + fib(n - 2);  // Рекурсивный вызов
    }
}