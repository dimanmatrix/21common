#include <stdio.h>

int scan_int_and_check(int qnt);
int largest_prime_divisor(long num);
int process(long num);

int is_even(long n);
long divide_with_subtract(long a, long b);
long modulo_with_subtract(long a, long b);
long is_prime(long n);

int main() { return scan_int_and_check(1); }

int scan_int_and_check(int req_qnt) {
    int qnt;
    long res;
    char after;

    if (req_qnt == 1) {
        qnt = scanf("%ld%c", &res, &after);
    } else {
        printf("Not implemented");
        return 1;
    }

    if (qnt != req_qnt + 1 || after != '\n') {
        printf("n/a");
        return 1;
    }

    return process(res);
}

int process(long num) {
    if (num < 0) {
        num = -num;
    }
    int res = largest_prime_divisor(num);
    if (num == -1) {
        printf("n/a");
        return 1;
    } else {
        printf("%d", res);
    }
    return 0;
}

int is_even(long num) {
    while (num > 1) {
        num -= 2;
    }
    return num == 0;
}

long divide_with_subtract(long a, long b) {
    int quotient = 0;
    while (a >= b) {
        a -= b;
        quotient++;
    }
    return quotient;
}

long modulo_with_subtract(long a, long b) {
    while (a >= b) {
        a -= b;
    }
    return a;
}

long is_prime(long num) {
    if (num <= 1) return 0;
    if (num == 2) return 1;
    if (is_even(num)) return 0;

    for (int i = 3; i * i <= num; i += 2) {
        if (modulo_with_subtract(num, i) == 0) {
            return 0;
        }
    }
    return 1;
}

int largest_prime_divisor(long num) {
    if (num <= 1) {
        return -1;
    }

    int largest = 1;
    for (int i = 2; i <= num; i++) {
        if (is_prime(i) && modulo_with_subtract(num, i) == 0 && i > largest) {
            largest = i;
        }
    }
    return largest;
}

/*
    OLD STUFF

int scan_2float();
int check_2float(int qnt, double num1, double num2, char ch);

int scan_2float() {
    double num1, num2;
    int qnt;
    char ch;

    qnt = scanf("%lf %lf%c", &num1, &num2, &ch);

    return check_float(qnt, num1, num2, ch);
}

int check_float(int qnt, double num1, double num2, char ch) {
    if (qnt == 3 && ch == '\n') {
        gotcha(num1, num2);
    } else {
        printf("n/a");
        return 1;
    }
}
*/