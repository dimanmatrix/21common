#include <math.h>
#include <stdio.h>

#define PI 3.14159265358979323846
#define STEPS 42
#define PLOT_HEIGHT 21
#define PLOT_WIDTH 42
char plot[PLOT_HEIGHT][PLOT_WIDTH];

double verziera(double x) { return 1 / (1 + x * x); }

double lemniscate(double x) {
    if (x < -1 || x > 1) return -1;
    double temp = sqrt(sqrt(1 + 4 * x * x) - 1) / sqrt(2);
    return temp;
}

double hyperbola(double x) {
    if (x == 0) return -1;
    return 1 / (x * x);
}

void init_plot() {
    for (int i = 0; i < PLOT_HEIGHT; i++) {
        for (int j = 0; j < PLOT_WIDTH; j++) {
            if (i == PLOT_HEIGHT / 2 && j == PLOT_WIDTH / 2) {
                plot[i][j] = '+';
            } else if (i == PLOT_HEIGHT / 2) {
                plot[i][j] = '-';
            } else if (j == PLOT_WIDTH / 2) {
                plot[i][j] = '|';
            } else {
                plot[i][j] = ' ';
            }
        }
    }
}

void plot_point(int x, int y, char c) {
    if (y >= 0 && y < PLOT_HEIGHT && x >= 0 && x < PLOT_WIDTH) {
        if (plot[y][x] == ' ' || plot[y][x] == '-' || plot[y][x] == '|') {
            plot[y][x] = c;
        }
    }
}

void display_plot() {
    for (int i = 0; i < PLOT_HEIGHT; i++) {
        for (int j = 0; j < PLOT_WIDTH; j++) {
            printf("%c", plot[i][j]);
        }
        printf("\n");
    }
}

void plot_function(double (*func)(double), char symbol, double min_y, double max_y) {
    for (int i = 0; i < PLOT_WIDTH; i++) {
        double x = -PI + (2 * PI * i) / (PLOT_WIDTH - 1);
        double y = func(x);
        if (y != -1 && y >= min_y && y <= max_y) {
            int plot_y = (int)((PLOT_HEIGHT - 1) * (1 - (y - min_y) / (max_y - min_y)));
            plot_point(i, plot_y, symbol);
        }
    }
}

int main() {
    double start = -PI;
    double end = PI;
    double step = (end - start) / (STEPS - 1);
    double x, v, l, h;

    if (1) {
        printf("\nТаблица:\n\n");
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
                printf("-\n");
            } else {
                printf("%.7f\n", h);
            }
        }
    }

    printf("\nГрафффиккк:\n\n");
    init_plot();
    // plot_function(verziera, '*', 0, 1);
    // plot_function(lemniscate, '+', 0, 1);
    plot_function(hyperbola, '#', 0, 10);
    display_plot();

    return 0;
}