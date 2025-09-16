# GAME_FIELD.C - Управление игровым полем

## Обзор модуля

`brick_game/tetris/game_field.c` реализует полную систему управления игровым полем:
- **Управление памятью** игрового поля (10×20) и матрицы next (4×4)
- **Алгоритмы очистки линий** с анимацией мигания
- **Проверка Game Over** условий
- **Клонирование поля** с наложением текущей фигуры
- **Оптимизированное управление памятью** через локальные буферы на стеке

## Структура игрового поля

### Размеры и константы
```c
#define FIELD_WIDTH 10          // Ширина поля (стандарт Тетрис)
#define FIELD_HEIGHT 20         // Высота поля (стандарт Тетрис)
#define NEXT_FIGURE_SIZE 4      // Размер матрицы next фигуры
```

### Представление поля
```
   0 1 2 3 4 5 6 7 8 9  ← X координаты (FIELD_WIDTH)
0  . . . . . . . . . .
1  . . . . . . . . . .
2  . . . . . . . . . .
3  . . . . . . . . . .
...
19 X X X X X X X X X X  ← Y координаты (FIELD_HEIGHT)
```

**Кодирование значений:**
- `0` - пустая клетка
- `1-7` - блоки фигур (тип фигуры + 1)
- `101-107` - мигающие блоки (значение + 100)

## Управление памятью поля

### tetris_create_field() - Создание игрового поля
**Файл:** `game_field.c:12-31`

```c
int **tetris_create_field(void) {
    // Выделяем массив указателей на строки
    int **field = malloc(FIELD_HEIGHT * sizeof(int *));
    if (!field) return NULL;

    // Выделяем каждую строку
    for (int i = 0; i < FIELD_HEIGHT; i++) {
        field[i] = malloc(FIELD_WIDTH * sizeof(int));
        if (!field[i]) {
            // КРИТИЧЕСКИ ВАЖНО: откат при ошибке
            for (int j = 0; j < i; j++) {
                free(field[j]);
            }
            free(field);
            return NULL;
        }
        // Инициализируем нулями (пустое поле)
        memset(field[i], 0, FIELD_WIDTH * sizeof(int));
    }

    return field;
}
```

**Архитектура памяти:**
```
field → [ptr0] → [int int int ... int]  ← строка 0 (10 int'ов)
        [ptr1] → [int int int ... int]  ← строка 1
        [ptr2] → [int int int ... int]  ← строка 2
        ...
        [ptr19]→ [int int int ... int]  ← строка 19
```

**Обработка ошибок:**
- При ошибке malloc любой строки → освобождаем уже выделенные
- Гарантирует отсутствие memory leaks
- Возвращает NULL при любой ошибке

### tetris_destroy_field() - Освобождение поля
**Файл:** `game_field.c:33-40`

```c
void tetris_destroy_field(int **field) {
    if (!field) return;

    // Освобождаем каждую строку
    for (int i = 0; i < FIELD_HEIGHT; i++) {
        free(field[i]);
    }
    // Освобождаем массив указателей
    free(field);
}
```

**Правильный порядок освобождения:**
1. Сначала освобождаем все строки `field[i]`
2. Затем освобождаем массив указателей `field`
3. Проверка на NULL предотвращает segfaults

### tetris_clear_field() - Очистка поля
**Файл:** `game_field.c:42-50`

```c
void tetris_clear_field(int **field) {
    if (!field) return;

    for (int y = 0; y < FIELD_HEIGHT; y++) {
        for (int x = 0; x < FIELD_WIDTH; x++) {
            field[y][x] = 0;  // Обнуляем все клетки
        }
    }
}
```

**Использование:**
- При рестарте игры `tetris_restart_game()`
- НЕ освобождает память - только обнуляет содержимое
- Эффективнее чем destroy + create при рестарте

## Система Next фигуры

### tetris_create_next_matrix() - Создание матрицы next
**Файл:** `game_field.c:56-74`

```c
int **tetris_create_next_matrix(void) {
    int **next = malloc(NEXT_FIGURE_SIZE * sizeof(int *));
    if (!next) return NULL;

    for (int i = 0; i < NEXT_FIGURE_SIZE; i++) {
        next[i] = malloc(NEXT_FIGURE_SIZE * sizeof(int));
        if (!next[i]) {
            // Аналогичный откат при ошибке
            for (int j = 0; j < i; j++) {
                free(next[j]);
            }
            free(next);
            return NULL;
        }
        memset(next[i], 0, NEXT_FIGURE_SIZE * sizeof(int));
    }

    return next;
}
```

**Размер матрицы next:**
- 4×4 элемента (достаточно для любой фигуры)
- Аналогичная архитектура памяти как основное поле
- Отдельное управление памятью

### tetris_update_next_matrix() - Обновление next матрицы
**Файл:** `game_field.c:85-105`

```c
void tetris_update_next_matrix(int **next, const Figure_t *figure) {
    if (!next || !figure) return;

    // Очищаем матрицу
    for (int y = 0; y < NEXT_FIGURE_SIZE; y++) {
        for (int x = 0; x < NEXT_FIGURE_SIZE; x++) {
            next[y][x] = 0;
        }
    }

    // Заполняем блоками фигуры
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (figure->blocks[y][x] != 0) {
                if (y < NEXT_FIGURE_SIZE && x < NEXT_FIGURE_SIZE) {
                    next[y][x] = figure->type + 1;  // +1 чтобы избежать 0
                }
            }
        }
    }
}
```

**Алгоритм:**
1. Полная очистка матрицы
2. Копирование только заполненных блоков из фигуры
3. Проверка границ (защита от overflow)
4. Кодирование: `figure->type + 1` (избегаем 0)

**Вызывается из FSM:**
- При создании новой фигуры в `STATE_SPAWN`
- Обновляет превью следующей фигуры для пользователя

## Система очистки линий

### tetris_find_full_lines() - Поиск заполненных линий
**Файл:** `game_field.c:111-137`

```c
int tetris_find_full_lines(int **field, int *lines_to_clear) {
    if (!field || !lines_to_clear) return 0;

    int count = 0;

    // Проверяем все линии снизу вверх
    for (int y = FIELD_HEIGHT - 1; y >= 0; y--) {
        bool line_full = true;

        // Проверяем заполнена ли линия (игнорируем мигающие блоки)
        for (int x = 0; x < FIELD_WIDTH; x++) {
            int cell_value = field[y][x];
            if (cell_value >= 100) cell_value -= 100;  // Убираем флаг мигания
            if (cell_value == 0) {
                line_full = false;
                break;
            }
        }

        if (line_full && count < 4) {  // Максимум 4 линии одновременно
            lines_to_clear[count] = y;
            count++;
        }
    }

    return count;
}
```

**Особенности алгоритма:**

#### 1. Обход снизу вверх
```c
for (int y = FIELD_HEIGHT - 1; y >= 0; y--)  // 19 → 18 → ... → 0
```
**Обоснование:** Естественный порядок - нижние линии очищаются первыми

#### 2. Обработка мигающих блоков
```c
int cell_value = field[y][x];
if (cell_value >= 100) cell_value -= 100;  // Убираем флаг мигания
if (cell_value == 0) {
    line_full = false;
    break;
}
```
**Логика мигания:**
- При анимации блоки получают +100: `1 → 101`, `7 → 107`
- Для проверки заполнения убираем флаг: `101 → 1`
- Позволяет корректно работать во время анимации

#### 3. Ограничение 4 линии
```c
if (line_full && count < 4) {  // Теоретический максимум в Тетрис
```

### tetris_clear_lines() - Очистка линий с гравитацией
**Файл:** `game_field.c:139-177`

```c
int tetris_clear_lines(int **field) {
    if (!field) return 0;

    int cleared_lines = 0;

    // Проверяем все линии снизу вверх
    for (int y = FIELD_HEIGHT - 1; y >= 0; y--) {
        bool line_full = true;

        // Проверяем заполнена ли линия (игнорируем мигающие блоки)
        for (int x = 0; x < FIELD_WIDTH; x++) {
            int cell_value = field[y][x];
            if (cell_value >= 100) cell_value -= 100;  // Убираем флаг мигания
            if (cell_value == 0) {
                line_full = false;
                break;
            }
        }

        if (line_full) {
            // Сдвигаем все линии выше на одну позицию вниз
            for (int move_y = y; move_y > 0; move_y--) {
                for (int x = 0; x < FIELD_WIDTH; x++) {
                    field[move_y][x] = field[move_y - 1][x];
                }
            }

            // Очищаем верхнюю линию
            for (int x = 0; x < FIELD_WIDTH; x++) {
                field[0][x] = 0;
            }

            cleared_lines++;
            y++;  // КРИТИЧЕСКИ ВАЖНО: проверяем эту же линию еще раз
        }
    }

    return cleared_lines;
}
```

**Алгоритм гравитации:**

#### 1. Сдвиг блоков вниз
```c
// Сдвигаем все линии выше на одну позицию вниз
for (int move_y = y; move_y > 0; move_y--) {
    for (int x = 0; x < FIELD_WIDTH; x++) {
        field[move_y][x] = field[move_y - 1][x];
    }
}
```

**Визуализация сдвига:**
```
До очистки линии 18:        После сдвига:
...                         ...
16: [X][X][ ][ ][X]        16: [ ][ ][ ][ ][ ]  ← новая пустая
17: [ ][ ][ ][ ][ ]        17: [X][X][ ][ ][X]  ← сдвинулось с 16
18: [X][X][X][X][X]        18: [ ][ ][ ][ ][ ]  ← сдвинулось с 17
19: [X][ ][X][ ][X]        19: [X][ ][X][ ][X]  ← не изменилось
```

#### 2. Очистка верхней линии
```c
// Очищаем верхнюю линию
for (int x = 0; x < FIELD_WIDTH; x++) {
    field[0][x] = 0;
}
```

#### 3. Повторная проверка
```c
y++;  // Проверяем эту же линию еще раз (так как сдвинули)
```
**Обоснование:** После сдвига в текущую позицию попала новая линия, которая тоже может быть заполнена

### Вспомогательные функции линий

#### tetris_is_line_full() - Проверка одной линии
**Файл:** `game_field.c:195-209`

```c
bool tetris_is_line_full(int **field, int line) {
    if (!field || line < 0 || line >= FIELD_HEIGHT) {
        return false;
    }

    for (int x = 0; x < FIELD_WIDTH; x++) {
        int cell_value = field[line][x];
        if (cell_value >= 100) cell_value -= 100;  // Убираем флаг мигания
        if (cell_value == 0) {
            return false;
        }
    }

    return true;
}
```

#### tetris_clear_line() - Очистка одной линии
**Файл:** `game_field.c:211-227`

```c
void tetris_clear_line(int **field, int line) {
    if (!field || line < 0 || line >= FIELD_HEIGHT) {
        return;
    }

    // Сдвигаем все линии выше на одну позицию вниз
    for (int y = line; y > 0; y--) {
        for (int x = 0; x < FIELD_WIDTH; x++) {
            field[y][x] = field[y - 1][x];
        }
    }

    // Очищаем верхнюю линию
    for (int x = 0; x < FIELD_WIDTH; x++) {
        field[0][x] = 0;
    }
}
```

## Проверка Game Over

### is_field_collision_at_top() - Проверка коллизии в топе
**Файл:** `game_field.c:179-193`

```c
bool is_field_collision_at_top(int **field) {
    if (!field) return false;

    // Проверяем верхние 2 линии (игнорируем мигающие блоки)
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < FIELD_WIDTH; x++) {
            int cell_value = field[y][x];
            if (cell_value >= 100) cell_value -= 100;  // Убираем флаг мигания
            if (cell_value != 0) {
                return true;  // Game Over
            }
        }
    }
    return false;
}
```

**Логика Game Over:**
- Проверяются только верхние 2 линии (y = 0, y = 1)
- Если любая клетка в этой зоне занята → Game Over
- Игнорируются мигающие блоки (корректная работа во время анимации)

**Использование в FSM:**
- Вызывается в `STATE_ATTACHING` после размещения фигуры
- Определяет переход: `STATE_SPAWN` или `STATE_GAME_OVER`

## Клонирование поля

### tetris_clone_field_and_add_current_figure() - Клонирование с фигурой
**Файл:** `game_field.c:233-300`

Критически важная функция для отображения игрового поля с наложенной текущей фигурой.

```c
void tetris_clone_field_and_add_current_figure(int **original_field,
                                               const TetrisState_t *state,
                                               int **output_buffer) {
    if (!original_field || !state || !output_buffer) {
        return;
    }

    // Копируем оригинальное поле в переданный буфер
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        for (int x = 0; x < FIELD_WIDTH; x++) {
            output_buffer[y][x] = original_field[y][x];
        }
    }

    // Накладываем текущую фигуру в активных состояниях
    if (state->fsm_state == STATE_MOVING ||
        state->fsm_state == STATE_DROP ||
        state->fsm_state == STATE_ATTACHING) {

        const Figure_t *current = &state->current_figure;

        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                if (current->blocks[y][x] != 0) {
                    int field_x = current->position.x + x;
                    int field_y = current->position.y + y;

                    // Проверяем границы поля
                    if (field_x >= 0 && field_x < FIELD_WIDTH &&
                        field_y >= 0 && field_y < FIELD_HEIGHT) {

                        // В состоянии ATTACHING перезаписываем даже занятые клетки
                        if (state->fsm_state == STATE_ATTACHING) {
                            output_buffer[field_y][field_x] = current->type + 1;
                        } else {
                            // В других состояниях накладываем только на пустые клетки
                            if (output_buffer[field_y][field_x] == 0) {
                                output_buffer[field_y][field_x] = current->type + 1;
                            }
                        }
                    }
                }
            }
        }
    }
}
```

### Архитектурные особенности

#### 1. Чистая функция без состояния
Функция принимает выходной буфер от вызывающего кода:
```c
void tetris_clone_field_and_add_current_figure(int **original_field,
                                               const TetrisState_t *state,
                                               int **output_buffer);
```

**Преимущества нового подхода:**
- **Нет глобального состояния** → thread-safe и reentrant
- **Память управляется извне** → четкая ответственность
- **Нет memory leaks** → память на стеке в вызывающем коде
- **Легко тестировать** → предсказуемое поведение

#### 2. Использование с локальным буфером
В `updateCurrentState()` создается локальный буфер:
```c
// Локальный буфер на стеке
int temp_field_data[FIELD_HEIGHT][FIELD_WIDTH];
int* temp_field_ptrs[FIELD_HEIGHT];

// Инициализация указателей
for (int i = 0; i < FIELD_HEIGHT; i++) {
    temp_field_ptrs[i] = temp_field_data[i];
}

// Заполнение буфера
tetris_clone_field_and_add_current_figure(original, state, temp_field_ptrs);
```

#### 3. Логика наложения фигуры

**Состояния когда показываем фигуру:**
- `STATE_MOVING` - фигура движется
- `STATE_DROP` - принудительное падение
- `STATE_ATTACHING` - во время анимации

**Специальная логика для STATE_ATTACHING:**
```c
if (state->fsm_state == STATE_ATTACHING) {
    // Перезаписываем даже занятые клетки, чтобы показать фигуру
    temp_field[field_y][field_x] = current->type + 1;
} else {
    // В других состояниях накладываем только на пустые клетки
    if (temp_field[field_y][field_x] == 0) {
        temp_field[field_y][field_x] = current->type + 1;
    }
}
```

**Обоснование:**
- В `STATE_ATTACHING` фигура уже размещена на поле FSM'ом
- Но визуально мы хотим показать её во время анимации мигания
- Поэтому перезаписываем блоки поля блоками фигуры

### Использование клонированного поля

**Основное назначение:**
- Вызывается из `updateCurrentState()` в `tetris.c`
- Заполняет переданный буфер полем с наложенной фигурой
- GUI видит фигуру как часть поля

**Жизненный цикл:**
1. FSM обновляет состояние (без изменения основного поля)
2. `updateCurrentState()` создает локальный буфер на стеке
3. Вызывает клонирование для заполнения буфера
4. Возвращает клон с фигурой для отображения
5. Буфер автоматически освобождается при выходе из функции
6. Основное поле остается чистым до `STATE_ATTACHING`

## Интеграция с другими модулями

### Взаимодействие с FSM

#### 1. Создание полей (tetris_init)
```c
g_tetris_state.public_info.field = tetris_create_field();
g_tetris_state.public_info.next = tetris_create_next_matrix();
```

#### 2. Очистка при рестарте
```c
tetris_clear_field(g_tetris_state.public_info.field);
```

#### 3. Анимация очистки (STATE_ATTACHING)
```c
// Поиск линий для анимации
state->animation_lines_count = tetris_find_full_lines(field, state->animation_lines);

// Очистка после анимации
int lines_cleared = tetris_clear_lines(state->public_info.field);
```

#### 4. Проверка Game Over
```c
if (is_field_collision_at_top(state->public_info.field)) {
    state->fsm_state = STATE_GAME_OVER;
}
```

### Взаимодействие с figures.c

#### 1. Проверка коллизий
```c
// figures.c использует field для проверки коллизий
bool tetris_check_collision(figure, field, offset_x, offset_y);
```

#### 2. Размещение фигур
```c
// figures.c размещает фигуры на field
void tetris_place_figure_on_field(figure, field);
```

#### 3. Обновление next матрицы
```c
// game_field.c обновляет превью
tetris_update_next_matrix(next, &state->next_figure);
```

## Производительность и оптимизация

### Критические функции
1. **`tetris_clone_field_and_add_current_figure()`** - вызывается каждый кадр
2. **`tetris_clear_lines()`** - сложная логика сдвига
3. **`tetris_find_full_lines()`** - поиск по всему полю

### Оптимизации

#### 1. Локальные буферы на стеке
- Избегаем malloc/free в критическом пути
- Память управляется автоматически
- Буферы создаются в вызывающей функции

#### 2. Эффективные циклы
```c
// Ранний выход при поиске незаполненных линий
for (int x = 0; x < FIELD_WIDTH; x++) {
    if (field[y][x] == 0) {
        line_full = false;
        break;  // Не проверяем остальные клетки
    }
}
```

#### 3. Обработка мигания
```c
// Единообразная обработка мигающих блоков
int cell_value = field[y][x];
if (cell_value >= 100) cell_value -= 100;
```

### Потенциальные улучшения

#### 1. Битовые операции
- Представить строку как 10-битное число
- Быстрая проверка заполнения через битовые маски

#### 2. Кэширование
- Кэшировать результаты `tetris_find_full_lines()`
- Инвалидировать кэш при изменении поля

#### 3. SIMD оптимизации
- Векторизованное копирование строк
- Параллельная проверка нескольких строк

#### 4. Thread safety
- Использование локальных буферов на стеке
- Отсутствие глобальных состояний
- Передача буферов через параметры функций

## Безопасность и надежность

### Проверки валидности
- Все функции проверяют входные параметры на NULL
- Проверки границ массивов
- Graceful degradation при ошибках

### Управление памятью
- Корректное освобождение при ошибках malloc
- Отсутствие memory leaks
- Безопасные локальные буферы на стеке

### Обработка крайних случаев
- Пустые поля и NULL указатели
- Выход за границы при проверке линий
- Некорректные координаты фигур