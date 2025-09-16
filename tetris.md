# TETRIS.C - Основной API и управление состоянием

## Обзор модуля

`brick_game/tetris/tetris.c` - центральный модуль библиотеки Tetris, реализующий:
- **API по спецификации** (`userInput`, `updateCurrentState`)
- **Управление глобальным состоянием** (единственная глобальная переменная)
- **Жизненный цикл игры** (init, destroy, restart)
- **Интеграцию с FSM и Storage**

## Глобальное состояние

### Единственная глобальная переменная
```c
static TetrisState_t g_tetris_state;  // Инкапсулированная в модуле
```

**Принципы дизайна:**
- `static` → видна только внутри `tetris.c`
- Инициализируется нулями автоматически
- Доступ только через функции-геттеры
- Никаких других глобальных переменных в проекте

### Структура TetrisState_t
```c
typedef struct {
    // Публичная часть по спецификации
    GameInfo_t public_info;

    // Внутренние данные FSM
    Figure_t current_figure;
    Figure_t next_figure;
    GameState_t fsm_state;
    GameStats_t stats;

    // Служебные данные
    bool initialized;
    long long last_move_time;

    // Анимация очистки линий
    int animation_lines[4];
    int animation_lines_count;
    int animation_counter;
    int animation_duration;
} TetrisState_t;
```

## Основной API (спецификация)

### userInput() - Обработка пользовательского ввода
**Файл:** `tetris.c:35-45`

```c
void userInput(UserAction_t action, bool hold) {
    // Автоматическая инициализация при первом вызове
    if (!is_game_initialized()) {
        if (!tetris_init()) {
            return;  // Ошибка инициализации
        }
    }

    // Передаем в FSM
    tetris_fsm_handle_input(action, hold);
}
```

**Алгоритм:**
1. **АВТОМАТИЧЕСКАЯ ИНИЦИАЛИЗАЦИЯ** при первом вызове любой API функции
2. Делегирование FSM для обработки логики
3. Никакой прямой работы с состоянием - только через FSM

**Поддерживаемые действия:**
- `Start` - старт игры / переключение паузы
- `Pause` - аналогично Start
- `Terminate` - принудительный Game Over
- `Left/Right` - горизонтальное движение
- `Up` - поворот фигуры
- `Down` - быстрое падение
- `Action` - рестарт игры

### updateCurrentState() - Получение текущего состояния
**Файл:** `tetris.c:47-66`

```c
GameInfo_t updateCurrentState(void) {
    GameInfo_t empty = {0};

    // Автоматическая инициализация при первом вызове
    if (!is_game_initialized()) {
        if (!tetris_init()) {
            return empty;  // Ошибка инициализации
        }
    }

    // Обновляем FSM
    tetris_fsm_update();

    // Получаем состояние
    TetrisState_t* state = tetris_get_state();
    if (!state) return empty;

    // Копируем публичную информацию
    GameInfo_t result = state->public_info;

    // Создаем локальный буфер на стеке для клонированного поля
    int temp_field_data[FIELD_HEIGHT][FIELD_WIDTH];
    int* temp_field_ptrs[FIELD_HEIGHT];

    // Инициализируем указатели
    for (int i = 0; i < FIELD_HEIGHT; i++) {
        temp_field_ptrs[i] = temp_field_data[i];
    }

    // Заполняем буфер клонированным полем с текущей фигурой
    tetris_clone_field_and_add_current_figure(state->public_info.field, state, temp_field_ptrs);

    // ЗАМЕНЯЕМ поле на клонированное
    result.field = temp_field_ptrs;

    return result;
}
```

**Критическая особенность:**
- Возвращает **клонированное поле** с наложенной текущей фигурой
- Оригинальное поле остается чистым (без текущей фигуры)
- FSM обновляется при каждом вызове

**Структура GameInfo_t (по спецификации):**
```c
typedef struct {
    int **field;        // 10x20 игровое поле
    int **next;         // 4x4 матрица следующей фигуры
    int score;          // Текущий счет
    int high_score;     // Лучший результат
    int level;          // Уровень (1-10)
    int speed;          // Скорость падения (мс)
    int pause;          // Состояние игры (int, НЕ bool!)
} GameInfo_t;
```

**КРИТИЧЕСКИ ВАЖНО: Поле pause используется для индикации состояний:**
- **pause = 0** - обычная игра (не на паузе)
- **pause = 1** - игра на паузе (пользователь нажал Pause)
- **pause = 6** - ⚠️ **GAME OVER** (фронт должен показать game over экран)
- **pause = 7** - ⚠️ **START SCREEN** (фронт должен показать start экран)

**Переходы состояний через pause:**
- При инициализации: FSM_STATE_START → pause = 7
- При нажатии Start: pause = 7 → pause = 0 (игра начинается)
- При завершении игры: любое состояние → pause = 6
- При паузе: pause = 0 ↔ pause = 1

## Жизненный цикл игры

### tetris_init() - Инициализация
**Файл:** `tetris.c:71-138`

**Алгоритм полной инициализации:**

#### 1. Проверка повторной инициализации
```c
if (is_game_initialized()) {
    return true;  // Уже инициализировано
}
```

#### 2. Сброс состояния
```c
memset(&g_tetris_state, 0, sizeof(TetrisState_t));
```

#### 3. Инициализация генератора случайных чисел
```c
struct timeval tv;
gettimeofday(&tv, NULL);
srand((unsigned int)tv.tv_usec);  // Микросекунды как seed
```

#### 4. Инициализация Storage системы
```c
if (!tetris_storage_init()) {
    #ifdef DEBUG
    printf("❌ INIT: Failed to initialize storage system\n");
    #endif
    return false;
}
```

#### 5. Создание игрового поля (10x20)
```c
g_tetris_state.public_info.field = tetris_create_field();
if (!g_tetris_state.public_info.field) {
    tetris_storage_cleanup();
    return false;
}
```

#### 6. Создание матрицы следующей фигуры (4x4)
```c
g_tetris_state.public_info.next = tetris_create_next_matrix();
if (!g_tetris_state.public_info.next) {
    tetris_destroy_field(g_tetris_state.public_info.field);
    tetris_storage_cleanup();
    return false;
}
```

#### 7. Инициализация игровых параметров
```c
g_tetris_state.public_info.score = 0;
g_tetris_state.public_info.high_score = tetris_sql_load_high_score();  // Из БД
g_tetris_state.public_info.level = 1;
g_tetris_state.public_info.speed = LEVEL_SPEEDS[0];  // 300ms
g_tetris_state.public_info.pause = 0;
```

#### 8. Инициализация внутренних данных
```c
g_tetris_state.fsm_state = STATE_START;
g_tetris_state.stats.lines_cleared = 0;
g_tetris_state.stats.figures_count = 0;
g_tetris_state.initialized = true;
g_tetris_state.last_move_time = clock();

// Сброс анимации
g_tetris_state.animation_lines_count = 0;
g_tetris_state.animation_counter = 0;
g_tetris_state.animation_duration = 0;
for (int i = 0; i < 4; i++) {
    g_tetris_state.animation_lines[i] = -1;
}
```

#### 9. Инициализация FSM
```c
tetris_fsm_init(&g_tetris_state.public_info);
```

**Обработка ошибок:**
- При любой ошибке происходит откат (cleanup)
- Освобождение уже выделенной памяти
- Очистка storage системы

### tetris_destroy() - Деинициализация
**Файл:** `tetris.c:140-170`

**Алгоритм полной очистки:**

#### 1. Проверка инициализации
```c
if (!is_game_initialized()) {
    return;  // Уже не инициализировано
}
```

#### 2. Финальное сохранение прогресса
```c
tetris_update_score(g_tetris_state.public_info.score,
                   g_tetris_state.stats.lines_cleared,
                   g_tetris_state.public_info.level);
```

#### 3. Очистка FSM
```c
tetris_fsm_destroy();
```

#### 4. Освобождение памяти полей
```c
if (g_tetris_state.public_info.field) {
    tetris_destroy_field(g_tetris_state.public_info.field);
    g_tetris_state.public_info.field = NULL;
}

if (g_tetris_state.public_info.next) {
    tetris_destroy_next_matrix(g_tetris_state.public_info.next);
    g_tetris_state.public_info.next = NULL;
}
```

#### 5. Очистка Storage системы
```c
tetris_storage_cleanup();
```

### tetris_library_cleanup() - Автоматическая очистка
**Файл:** `tetris.c:147-149`

```c
// Автоматическое освобождение ресурсов при выгрузке библиотеки
void __attribute__((destructor)) tetris_library_cleanup(void) {
    tetris_destroy();
}
```

**Назначение:**
- **Автоматически вызывается** при выгрузке динамической библиотеки (`dlclose()`)
- Гарантирует освобождение всех ресурсов даже если фронт "забыл" вызвать деструктор
- Использует GCC атрибут `__attribute__((destructor))`
- Безопасно для многократного вызова (tetris_destroy проверяет инициализацию)

### tetris_restart_game() - Перезапуск
**Файл:** `tetris.c:176-216`

**Алгоритм "мягкого" перезапуска:**

#### 1. Сохранение текущего прогресса
```c
tetris_update_score(g_tetris_state.public_info.score,
                   g_tetris_state.stats.lines_cleared,
                   g_tetris_state.public_info.level);
```

#### 2. Очистка игрового поля
```c
tetris_clear_field(g_tetris_state.public_info.field);
```

#### 3. Сброс игровой статистики
```c
int saved_high_score = tetris_sql_load_high_score();  // Перезагружаем актуальный рекорд
g_tetris_state.public_info.score = 0;
g_tetris_state.public_info.high_score = saved_high_score;
g_tetris_state.public_info.level = 1;
g_tetris_state.public_info.speed = 1000;
g_tetris_state.public_info.pause = 0;

g_tetris_state.fsm_state = STATE_START;
g_tetris_state.stats.lines_cleared = 0;
g_tetris_state.stats.figures_count = 0;
```

#### 4. Сброс анимации
```c
g_tetris_state.animation_lines_count = 0;
g_tetris_state.animation_counter = 0;
g_tetris_state.animation_duration = 0;
for (int i = 0; i < 4; i++) {
    g_tetris_state.animation_lines[i] = -1;
}
```

#### 5. Переинициализация FSM
```c
tetris_fsm_init(&g_tetris_state.public_info);
```

**Отличие от полной инициализации:**
- НЕ пересоздает поля (memory reuse)
- НЕ переинициализирует Storage
- НЕ сбрасывает генератор случайных чисел
- Быстрее и безопаснее

## Вспомогательные функции

### is_game_initialized() - Проверка инициализации
**Файл:** `tetris.c:27-29`

```c
static bool is_game_initialized(void) {
    return g_tetris_state.public_info.field != NULL;
}
```

**Логика проверки:**
- Если поле создано → игра инициализирована
- Простая и надежная проверка
- Используется во всех публичных функциях

### tetris_get_state() - Геттер состояния
**Файл:** `tetris.c:298-300`

```c
TetrisState_t* tetris_get_state(void) {
    return is_game_initialized() ? &g_tetris_state : NULL;
}
```

**Назначение:**
- Инкапсулированный доступ к состоянию для FSM
- Возвращает NULL если не инициализировано
- Единственный способ получить полное состояние

### tetris_is_game_over() - Проверка окончания
**Файл:** `tetris.c:172-174`

```c
bool tetris_is_game_over(void) {
    return is_game_initialized() && (g_tetris_state.fsm_state == STATE_GAME_OVER);
}
```

### get_current_time_ms() - Текущее время
**Файл:** `tetris.c:18-22`

```c
long long get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
}
```

**Использование:**
- Таймеры автопадения в FSM
- Seed для генератора случайных чисел
- Высокая точность (микросекунды → миллисекунды)

## Real-time Storage интеграция

### tetris_update_current_score() - Живое обновление счета
**Файл:** `tetris.c:221-241`

```c
void tetris_update_current_score(void) {
    if (!is_game_initialized()) {
        return;
    }

    // Обновляем текущий счет в хранилище
    bool success = tetris_update_score(g_tetris_state.public_info.score,
                                      g_tetris_state.stats.lines_cleared,
                                      g_tetris_state.public_info.level);

    // Обновляем high_score если нужно
    if (success) {
        int current_high = tetris_sql_load_high_score();
        if (current_high > g_tetris_state.public_info.high_score) {
            g_tetris_state.public_info.high_score = current_high;
        }
    }
}
```

**Назначение:**
- Вызывается из FSM при изменении счета/уровня
- Синхронизирует локальный high_score с базой данных
- Обеспечивает актуальность рекордов между сессиями

**Точки вызова:**
1. При повышении уровня (`update_level` в FSM)
2. При очистке линий (`add_score_for_lines` в FSM)
3. При Game Over (`fsm_state_game_over` в FSM)

## DEBUG система

### tetris_debug_print_state() - Полная диагностика
**Файл:** `tetris.c:243-292`

**Выводит полную информацию о состоянии:**
- Состояние FSM и флаги инициализации
- Текущая и следующая фигуры
- Игровая статистика и счета
- Таймеры и время

```c
#ifdef DEBUG
void tetris_debug_print_state(void) {
    printf("\n🔍 =========================\n");
    printf("   TETRIS DEBUG STATE\n");
    printf("   =========================\n");

    if (!is_game_initialized()) {
        printf("❌ Game NOT initialized!\n");
        return;
    }

    TetrisState_t* state = &g_tetris_state;

    // FSM состояние
    printf("🎮 FSM State: ");
    switch (state->fsm_state) {
        case STATE_START:     printf("START\n"); break;
        case STATE_SPAWN:     printf("SPAWN\n"); break;
        // ... остальные состояния
    }

    // Фигуры
    printf("\n📦 Current Figure:\n");
    printf("   Type: %d, Position: (%d, %d), Rotation: %d\n",
           state->current_figure.type,
           state->current_figure.position.x,
           state->current_figure.position.y,
           state->current_figure.rotation);

    // Статистика
    printf("\n📊 Game Stats:\n");
    printf("   Score: %d, High Score: %d, Level: %d\n",
           state->public_info.score,
           state->public_info.high_score,
           state->public_info.level);

    // Таймеры
    printf("\n⏱️  Timing:\n");
    printf("   Last Move Time: %lld, Current Time: %ld\n",
           state->last_move_time, clock());
}
#endif
```

## Архитектурные принципы

### Инкапсуляция
- Единственная глобальная переменная
- Доступ только через публичные функции
- Внутренние детали скрыты от внешнего кода

### Разделение ответственности
- **tetris.c**: Управление состоянием и API
- **fsm.c**: Игровая логика и переходы состояний
- **figures.c**: Операции с фигурами
- **game_field.c**: Операции с игровым полем

### Обработка ошибок
- Проверка инициализации в каждой функции
- Graceful degradation при ошибках Storage
- Корректная очистка ресурсов при ошибках

### Memory Management
- Контролируемое выделение памяти в init
- Корректное освобождение в destroy
- Переиспользование памяти в restart
- Никаких memory leaks

### Backward Compatibility
- Строгое соблюдение API спецификации
- Неизменность структуры GameInfo_t
- Совместимость с будущими GUI интерфейсами