# DISPLAY.H/C - Отрисовка игрового интерфейса

## Обзор модуля

`gui/cli/display.c` реализует полную систему отрисовки игрового интерфейса:
- **Полноэкранная отрисовка** игрового поля с рамками
- **Цветовая схема** для разных типов фигур
- **Боковая панель** с игровой информацией и превью
- **Специальные экраны** (старт, game over)
- **ANSI позиционирование** курсора для оптимизации

## Константы отображения

### Размеры интерфейса
**Файл:** `display.h:14-16`

```c
#define FIELD_DISPLAY_WIDTH (FIELD_WIDTH * 2)  // 20 символов (10 * 2)
#define SIDEBAR_WIDTH 20                       // Боковая панель
#define TOTAL_DISPLAY_WIDTH (FIELD_DISPLAY_WIDTH + SIDEBAR_WIDTH + 3)  // 43
```

**Логика вычислений:**
- Игровое поле: 10×20 → 20×20 символов (каждый "пиксель" = 2 символа)
- Боковая панель: 20 символов для информации
- Разделители: 3 символа (левая граница + правая граница + разделитель)

### Символы отрисовки
**Файл:** `display.h:19-24`

```c
#define EMPTY_CELL "  "           // Пустая клетка (2 пробела)
#define FILLED_CELL "██"          // Заполненная клетка (2 блока)
#define BORDER_CHAR "│"           // Вертикальная граница
#define TOP_BORDER "┌"            // Верхний левый угол
#define BOTTOM_BORDER "└"         // Нижний левый угол
#define HORIZONTAL "─"            // Горизонтальная линия
```

**Unicode символы:**
- `│` (U+2502) - вертикальная линия
- `┌` (U+250C) - верхний левый угол
- `└` (U+2514) - нижний левый угол
- `─` (U+2500) - горизонтальная линия
- `██` (U+2588) - полный блок × 2

## Цветовая схема

### display_get_figure_color() - Цвета фигур
**Файл:** `display.c:80-91`

```c
const char* display_get_figure_color(int cell_value) {
    switch (cell_value) {
        case 1: return CYAN;     // I-фигура (палка)
        case 2: return YELLOW;   // O-фигура (квадрат)
        case 3: return MAGENTA;  // T-фигура
        case 4: return GREEN;    // S-фигура
        case 5: return RED;      // Z-фигура
        case 6: return BLUE;     // J-фигура
        case 7: return WHITE;    // L-фигура
        default: return RESET;
    }
}
```

**Соответствие значений поля типам фигур:**
```c
// Из figures.c: field[y][x] = figure->type + 1
FIGURE_I (0) + 1 = 1 → CYAN
FIGURE_O (1) + 1 = 2 → YELLOW
FIGURE_T (2) + 1 = 3 → MAGENTA
FIGURE_S (3) + 1 = 4 → GREEN
FIGURE_Z (4) + 1 = 5 → RED
FIGURE_J (5) + 1 = 6 → BLUE
FIGURE_L (6) + 1 = 7 → WHITE
```

**Дизайн решения:**
- **Контрастные цвета** для легкого различения
- **Традиционная Тетрис палитра** (I=голубой, O=желтый)
- **RESET** для неизвестных значений (безопасность)

## Главная функция отрисовки

### display_draw_field() - Отрисовка игрового поля
**Файл:** `display.c:93-145`

```c
void display_draw_field(GameInfo_t *game_info, bool is_game_over, bool is_initialized) {
    printf(CURSOR_HOME);  // Курсор в начало (\033[H)

    // Заголовок
    printf(BOLD CYAN "    " TOP_BORDER);
    for (int i = 0; i < FIELD_DISPLAY_WIDTH; i++) {
        printf(HORIZONTAL);
    }
    printf(" TETRIS " RESET "\n");

    // Основное поле с боковой панелью
    for (int row = 0; row < FIELD_HEIGHT; row++) {
        printf("    " BORDER_CHAR);  // Левая граница

        // Игровое поле
        for (int col = 0; col < FIELD_WIDTH; col++) {
            int cell = game_info->field[row][col];
            if (cell == 0) {
                printf(EMPTY_CELL);      // "  "
            } else {
                printf("%s%s%s", display_get_figure_color(cell), FILLED_CELL, RESET);
            }
        }

        printf(BORDER_CHAR);  // Правая граница поля

        // Боковая панель
        draw_sidebar(game_info, row);

        printf("\n");
    }

    // Отрисовка следующей фигуры поверх боковой панели
    if (game_info->next) {
        draw_next_figure(game_info->next, 8);
    }

    // Нижняя граница
    printf("    " BOTTOM_BORDER);
    for (int i = 0; i < FIELD_DISPLAY_WIDTH; i++) {
        printf(HORIZONTAL);
    }
    printf(" Q-quit " RESET "\n");

    // Статус игры
    if (is_game_over) {
        printf("\n" BOLD RED "    GAME OVER! Press S to start new game" RESET "\n");
    } else if (!is_initialized) {
        printf("\n" BOLD GREEN "    Press S to start the game" RESET "\n");
    }

    fflush(stdout);
}
```

### Архитектура отрисовки

#### 1. Позиционирование курсора
```c
printf(CURSOR_HOME);  // "\033[H" - позиция (1,1)
```
**Эффект:** Перерисовка поверх предыдущего кадра без мерцания

#### 2. Заголовок с рамкой
```
    ┌────────────────────── TETRIS
```
**Структура:** Отступ + верхний угол + горизонтальные линии + название

#### 3. Основное поле (20 строк)
```
    │██  ██    ██      ██│ Score: 1200
    │    ██  ██  ██    ██│ High: 2500
    │                  ██│ Level: 3
```
**Структура:** Отступ + граница + поле + граница + боковая панель

#### 4. Нижняя граница
```
    └────────────────────── Q-quit
```

#### 5. Статусные сообщения
Динамическое отображение в зависимости от состояния игры.

## Боковая панель

### draw_sidebar() - Информационная панель
**Файл:** `display.c:34-74`

```c
static void draw_sidebar(GameInfo_t *game_info, int row) {
    switch (row) {
        case 2:
            printf(" Score: %d", game_info->score);
            break;
        case 3:
            printf(" High: %d", game_info->high_score);
            break;
        case 4:
            printf(" Level: %d", game_info->level);
            break;
        case 5:
            printf(" Speed: %d", game_info->speed);
            break;
        case 7:
            printf(" Next:");
            break;
        case 8: case 9: case 10: case 11:
            // Фигура отрисовывается отдельно через draw_next_figure()
            break;
        case 13:
            if (game_info->pause) {
                printf(YELLOW BOLD " PAUSE" RESET);
            } else {
                printf(" Playing");
            }
            break;
        case 16:
            printf(" Controls:");
            break;
        case 17:
            printf(" WASD/Arrows");
            break;
        case 18:
            printf(" R/E - Rotate");
            break;
        case 19:
            printf(" P - Pause");
            break;
    }
}
```

### Макет боковой панели

```
Строка:  Содержимое:
   0     (пустая)
   1     (пустая)
   2     Score: 1200
   3     High: 2500
   4     Level: 3
   5     Speed: 300
   6     (пустая)
   7     Next:
 8-11    [4x4 превью фигуры]
  12     (пустая)
  13     PAUSE / Playing
  14     (пустая)
  15     (пустая)
  16     Controls:
  17     WASD/Arrows
  18     R/E - Rotate
  19     P - Pause
```

**Дизайн принципы:**
- **Статическая раскладка** - каждая строка имеет фиксированное назначение
- **Выравнивание по левому краю** с отступом в 1 пробел
- **Группировка информации** - счет, управление, помощь
- **Цветовое выделение** для статуса паузы

## Превью следующей фигуры

### draw_next_figure() - Отрисовка next фигуры
**Файл:** `display.c:13-31`

```c
static void draw_next_figure(int **next_matrix, int start_row) {
    for (int row = 0; row < NEXT_FIGURE_SIZE; row++) {
        printf("\033[%d;%dH", start_row + row + 1, FIELD_DISPLAY_WIDTH + 7);
        // Позиционируем курсор в нужное место боковой панели
        printf(" ");

        if (next_matrix) {
            for (int col = 0; col < NEXT_FIGURE_SIZE; col++) {
                int cell = next_matrix[row][col];
                if (cell == 0) {
                    printf("  ");
                } else {
                    printf("%s██%s", display_get_figure_color(cell), RESET);
                }
            }
        } else {
            printf("        "); // Пустое место если нет фигуры
        }
    }
}
```

### ANSI позиционирование курсора

#### Формула позиции
```c
printf("\033[%d;%dH", start_row + row + 1, FIELD_DISPLAY_WIDTH + 7);
//                    ^^^^^^^^^^^^^^^^^   ^^^^^^^^^^^^^^^^^^^^^^^^^^^
//                    Y координата       X координата
```

**Вычисления:**
- `start_row = 8` (строка "Next:")
- `row = 0,1,2,3` (4 строки фигуры)
- `Y = 8 + 0 + 1 = 9` (первая строка фигуры)
- `X = 20 + 7 = 27` (после игрового поля + отступ)

#### Преимущества прямого позиционирования
- **Независимость от основного потока** отрисовки
- **Перерисовка поверх панели** без структурных изменений
- **Оптимизация** - обновляем только область фигуры

## Специальные экраны

### display_draw_start_screen() - Стартовый экран
**Файл:** `display.c:147-174`

```c
void display_draw_start_screen(void) {
    terminal_clear_screen();

    printf(BOLD CYAN "\n");
    printf("    ██████╗ ██████╗ ██╗ ██████╗██╗  ██╗ ██████╗  █████╗ ███╗   ███╗███████╗\n");
    printf("    ██╔══██╗██╔══██╗██║██╔════╝██║ ██╔╝██╔════╝ ██╔══██╗████╗ ████║██╔════╝\n");
    printf("    ██████╔╝██████╔╝██║██║     █████╔╝ ██║  ███╗███████║██╔████╔██║█████╗  \n");
    printf("    ██╔══██╗██╔══██╗██║██║     ██╔═██╗ ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝  \n");
    printf("    ██████╔╝██║  ██║██║╚██████╗██║  ██╗╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗\n");
    printf("    ╚═════╝ ╚═╝  ╚═╝╚═╝ ╚═════╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝\n");
    printf(RESET "\n");

    printf(BOLD WHITE "                              TETRIS GAME\n" RESET);
    printf("                    Добро пожаловать в BrickGame Tetris!\n\n");

    printf(GREEN "    Управление:\n");
    printf("      ↑ / W / R / E  - Поворот фигуры\n");
    printf("      ← / A          - Движение влево\n");
    printf("      → / D          - Движение вправо\n");
    printf("      ↓ / S          - Ускоренное падение\n");
    printf("      P / Space      - Пауза\n");
    printf("      Q / Esc        - Выход\n" RESET);

    printf("\n" BOLD YELLOW "    Нажмите S или Enter для начала игры" RESET "\n");
    printf("    Нажмите Q или Esc для выхода\n\n");

    fflush(stdout);
}
```

**ASCII Art логотип:**
- **"BRICKGAME"** из Unicode символов
- **Точные пропорции** для терминального шрифта
- **Цветовое выделение** (голубой + жирный)

**Структура экрана:**
1. Полная очистка экрана
2. ASCII логотип с отступами
3. Приветственное сообщение
4. Справка по управлению
5. Инструкции для продолжения

### display_draw_game_over_screen() - Экран окончания
**Файл:** `display.c:176-196`

```c
void display_draw_game_over_screen(int final_score, int high_score) {
    printf("\n\n" BOLD RED);
    printf("     ██████╗  █████╗ ███╗   ███╗███████╗     ██████╗ ██╗   ██╗███████╗██████╗ \n");
    printf("    ██╔════╝ ██╔══██╗████╗ ████║██╔════╝    ██╔═══██╗██║   ██║██╔════╝██╔══██╗\n");
    printf("    ██║  ███╗███████║██╔████╔██║█████╗      ██║   ██║██║   ██║█████╗  ██████╔╝\n");
    printf("    ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝      ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗\n");
    printf("    ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗    ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║\n");
    printf("     ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝     ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝\n");
    printf(RESET "\n");

    printf(BOLD WHITE "                            Ваш результат: %d\n", final_score);
    if (final_score >= high_score) {
        printf(BOLD YELLOW "                         🎉 НОВЫЙ РЕКОРД! 🎉\n" RESET);
    } else {
        printf("                            Лучший счет: %d\n", high_score);
    }
    printf(RESET "\n");

    printf(BOLD GREEN "              Нажмите S для новой игры или Q для выхода\n" RESET);
    fflush(stdout);
}
```

**Особенности:**
- **"GAME OVER"** ASCII art (красный + жирный)
- **Условное поздравление** при новом рекорде (эмодзи + желтый)
- **Сравнение результатов** текущий vs лучший
- **Инструкции продолжения** игры

### display_draw_status_line() - Строка состояния
**Файл:** `display.c:198-201`

```c
void display_draw_status_line(const char* message) {
    printf("\n" BOLD CYAN "    %s" RESET "\n", message);
    fflush(stdout);
}
```

**Назначение:**
- Временные сообщения пользователю
- Отладочная информация
- Системные уведомления

## Производительность и оптимизация

### Эффективность отрисовки

#### 1. Позиционирование курсора
```c
printf(CURSOR_HOME);  // Один раз в начале кадра
```
**Эффект:** Перерисовка поверх предыдущего содержимого без очистки экрана

#### 2. Минимальные ANSI команды
- Цвета применяются только к блокам фигур
- Сброс цвета (`RESET`) только когда необходимо
- Одна команда `fflush()` в конце

#### 3. Статическое позиционирование
Боковая панель использует фиксированные позиции строк, что исключает сложные вычисления.

### Частота обновления
```c
// В main.c:
#define REFRESH_RATE_MS 50  // 20 FPS
```

**Нагрузка:** Около 1000 printf() вызовов в секунду при активной игре.

### Потенциальные оптимизации

#### 1. Буферизация вывода
```c
static char display_buffer[4096];
// Формирование кадра в памяти, затем одна запись в stdout
```

#### 2. Dirty regions
Обновление только измененных областей экрана вместо полной перерисовки.

#### 3. Двойная буферизация
Переключение между двумя экранными буферами для устранения мерцания.

#### 4. Компрессия ANSI команд
Группировка нескольких цветовых переключений в одну последовательность.

## Совместимость и портируемость

### Поддержка терминалов
- ✅ **xterm, GNOME Terminal, iTerm2, macOS Terminal**
- ✅ **SSH сессии** (при правильном TERM)
- ⚠️ **Windows Command Prompt** (требует включения ANSI)
- ❌ **Файловые перенаправления**

### Unicode символы
```c
// Требуют UTF-8 поддержки терминала
#define BORDER_CHAR "│"    // U+2502
#define FILLED_CELL "██"   // U+2588 × 2
```

**Fallback стратегия:**
```c
// При отсутствии Unicode поддержки
#define BORDER_CHAR "|"
#define FILLED_CELL "##"
```

### Размеры терминала
**Минимальные требования:**
- Ширина: 43 символа (`TOTAL_DISPLAY_WIDTH`)
- Высота: 25 строк (поле + заголовки + статус)

**Проверка размера:**
```bash
# Получение размеров терминала
tput cols  # Ширина
tput lines # Высота
```

## Интеграция с другими модулями

### Зависимости
- **terminal.h** - ANSI константы и управление курсором
- **tetris.h** - структуры `GameInfo_t` и константы поля
- **input.h** - не используется напрямую, но связан логически

### Использование в main.c
```c
// Инициализация
display_draw_start_screen();

// Игровой цикл
GameInfo_t game_info = tetris_lib->updateCurrentState();
display_draw_field(&game_info, is_game_over, game_initialized);

// Завершение
display_draw_game_over_screen(final_score, high_score);
```

## Возможные расширения

### Анимация и эффекты
1. **Мигание при очистке линий** (уже реализовано в FSM)
2. **Плавное падение фигур**
3. **Частицы при очистке линий**
4. **Анимированные переходы экранов**

### Дополнительная информация
1. **Статистика по типам фигур**
2. **Время игры**
3. **Очищенные линии по типам** (одиночные, двойные, etc.)
4. **Графики производительности**

### Настройки отображения
1. **Выбор цветовой схемы**
2. **Изменение символов отрисовки**
3. **Масштабирование интерфейса**
4. **Темы оформления**