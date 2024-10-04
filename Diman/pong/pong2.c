#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_X 80
#define MAX_Y 25

typedef struct {
    int x, y;
    int speed_x, speed_y;
} Ball;

typedef struct {
    int start, end;
    int x;
} Racket;

typedef struct {
    Ball ball;
    Racket left_racket, right_racket;
    int left_score, right_score;
} GameState;

void init_game(GameState *game) {
    game->ball.x = MAX_X / 2;
    game->ball.y = MAX_Y / 2;
    game->ball.speed_x = 1;
    game->ball.speed_y = 1;

    game->left_racket.start = 5;
    game->left_racket.end = 7;
    game->left_racket.x = 3;

    game->right_racket.start = 17;
    game->right_racket.end = 19;
    game->right_racket.x = MAX_X - 4;

    game->left_score = game->right_score = 0;
}

void draw_vertical_line(int x, int start, int end) {
    for (int y = start; y <= end; y++) {
        printf("\033[%d;%dH|", y, x);
    }
}

void draw_game(GameState *game) {
    printf("\033[2J"); // Clear screen

    // Draw borders
    for (int i = 0; i < MAX_X; i++) {
        printf("\033[1;%dH-", i + 1);
        printf("\033[%d;%dH-", MAX_Y, i + 1);
    }
    for (int i = 0; i < MAX_Y; i++) {
        printf("\033[%d;1H|", i + 1);
        printf("\033[%d;%dH|", i + 1, MAX_X);
    }

    // Draw rackets
    draw_vertical_line(game->left_racket.x, game->left_racket.start, game->left_racket.end);
    draw_vertical_line(game->right_racket.x, game->right_racket.start, game->right_racket.end);

    // Draw ball
    printf("\033[%d;%dH*", game->ball.y, game->ball.x);

    // Draw scores
    printf("\033[%d;%dHLeft: %d  Right: %d", MAX_Y + 1, 1, game->left_score, game->right_score);
}

void update_game(GameState *game, char input) {
    // Update ball position
    game->ball.x += game->ball.speed_x;
    game->ball.y += game->ball.speed_y;

    // Ball collision with top and bottom
    if (game->ball.y <= 1 || game->ball.y >= MAX_Y - 1) {
        game->ball.speed_y = -game->ball.speed_y;
    }

    // Ball collision with rackets
    if ((game->ball.x == game->left_racket.x + 1 && 
         game->ball.y >= game->left_racket.start && 
         game->ball.y <= game->left_racket.end) ||
        (game->ball.x == game->right_racket.x - 1 && 
         game->ball.y >= game->right_racket.start && 
         game->ball.y <= game->right_racket.end)) {
        game->ball.speed_x = -game->ball.speed_x;
    }

    // Scoring
    if (game->ball.x <= 1) {
        game->right_score++;
        game->ball.x = MAX_X / 2;
        game->ball.y = MAX_Y / 2;
    } else if (game->ball.x >= MAX_X - 1) {
        game->left_score++;
        game->ball.x = MAX_X / 2;
        game->ball.y = MAX_Y / 2;
    }

    // Racket movement
    switch(input) {
        case 'a':
            if (game->left_racket.start > 1) {
                game->left_racket.start--;
                game->left_racket.end--;
            }
            break;
        case 'z':
            if (game->left_racket.end < MAX_Y - 2) {
                game->left_racket.start++;
                game->left_racket.end++;
            }
            break;
        case 'k':
            if (game->right_racket.start > 1) {
                game->right_racket.start--;
                game->right_racket.end--;
            }
            break;
        case 'm':
            if (game->right_racket.end < MAX_Y - 2) {
                game->right_racket.start++;
                game->right_racket.end++;
            }
            break;
    }
}

int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

int main(void) {
    GameState game;
    init_game(&game);

    while (1) {
        draw_game(&game);

        char input = 0;
        if (kbhit()) {
            input = getchar();
            if (input == 'q') break;
        }

        update_game(&game, input);

        usleep(50000);  // Задержка для контроля скорости игры

        if (game.left_score >= 20 || game.right_score >= 20) {
            printf("\033[%d;%dHGame Over! %s wins!", MAX_Y + 2, 1, 
                   game.left_score >= 20 ? "Left player" : "Right player");
            break;
        }
    }

    return 0;
}