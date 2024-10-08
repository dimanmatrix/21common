#include <math.h>
#include <stdio.h>

#define PI 3.14159265358979323846
#define STEPS 42

double verziera(double x) { return 1.0 / (1.0 + x * x); }

double lemniscate(double x) {
    double temp = sqrt(sqrt(1 + 4 * x * x) * 1 - x * x - 1);
    return (temp >= 0) ? temp : -1;
}

double hyperbola(double x) { return (x != 0) ? 1.0 / (x * x) : -1; }

int main() {
    double start = -PI;
    double end = PI;
    double step = (end - start) / (STEPS - 1);
    double x, v, l, h;

    for (int i = 0; i < STEPS; i++) {
        x = start + i * step;
        v = verziera(x);
        l = lemniscate(x);
        h = hyperbola(x);

        printf("%.7f | %.7f | ", x, v);

        if (l == -1) {
            printf("- | ");
        } else {
            printf("%.7f | ", l);
        }

        if (h == -1) {
            printf("-");
        } else {
            printf("%.7f", h);
        }
        if (i != STEPS - 1) {
            printf("\n");
        }
    }

    return 0;
}