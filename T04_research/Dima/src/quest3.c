#include <stdio.h>

long long fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    int n;
    if (scanf("%d", &n) != 1 || n < 0) {
        printf("n/a");
        return 1;
    }

    long long result = fibonacci(n);
    printf("%lld", result);

    return 0;
}