# FSM (Finite State Machine) - Конечный автомат Тетрис

## Обзор архитектуры

FSM является сердцем игры Тетрис, реализуя 6 основных состояний игрового процесса. Вся логика состояний сосредоточена в файле `brick_game/tetris/fsm.c`.

## Структура состояний

```
START → SPAWN → MOVING ⟷ DROP → ATTACHING → SPAWN
   ↑                                ↓
   └──────────── GAME_OVER ←────────┘
```

## ⚠️ КРИТИЧЕСКАЯ СВЯЗЬ FSM С ПОЛЕМ PAUSE

FSM состояния **напрямую влияют на поле pause** в GameInfo_t для координации с фронтом:

| FSM Состояние | Значение pause | Что должен показать фронт |
|---------------|----------------|---------------------------|
| `STATE_START` | **pause = 7** | 🎮 Start screen ("Нажмите S для начала") |
| `STATE_SPAWN` | pause = 0 | 🎯 Игровое поле |
| `STATE_MOVING` | pause = 0/1 | 🎯 Игровое поле / ⏸️ Пауза |
| `STATE_DROP` | pause = 0 | 🎯 Игровое поле |
| `STATE_ATTACHING` | pause = 0 | 🎯 Игровое поле |
| `STATE_GAME_OVER` | **pause = 6** | 💀 Game Over screen ("Нажмите S для рестарта") |

**Почему это критично:**
- Фронт **НЕ ЗНАЕТ** внутренних FSM состояний (только API: userInput/updateCurrentState)
- Поле `pause` - **ЕДИНСТВЕННЫЙ способ** для фронта понять что показывать
- `pause = 6/7` - специальные значения для экранов, НЕ для паузы игры!

### Состояния FSM

| Состояние | Описание | Переходы |
|-----------|----------|----------|
| `STATE_START` | Ожидание начала игры | → SPAWN (по Start/Pause) |
| `STATE_SPAWN` | Создание новой фигуры | → MOVING или GAME_OVER |
| `STATE_MOVING` | Основное игровое состояние | → DROP, ATTACHING |
| `STATE_DROP` | Принудительное падение | → ATTACHING |
| `STATE_ATTACHING` | Анимация + прикрепление | → SPAWN или GAME_OVER |
| `STATE_GAME_OVER` | Конец игры | → SPAWN (рестарт) |

## Детальное описание состояний

### 1. STATE_START
**Файл:** `fsm.c:142-147`

Стартовое состояние - устанавливает pause=7 для индикации start screen.
```c
static void fsm_state_start(TetrisState_t* state) {
    if (!state) return;

    // Устанавливаем pause = 7 для индикации start screen
    state->public_info.pause = 7;
}
```

**⚠️ КРИТИЧНО ДЛЯ ФРОНТА:** `pause = 7` означает что нужно показать start screen!
**Выход из состояния:** При получении `Start` или `Pause` → переход в `STATE_SPAWN`

### 2. STATE_SPAWN
**Файл:** `fsm.c:144-169`

Создание новых фигур и проверка Game Over.

**Алгоритм:**
1. Перемещение `next_figure` → `current_figure`
2. Генерация новой `next_figure`
3. Обновление матрицы предпросмотра (4x4)
4. **Критическая проверка:** коллизия при появлении = Game Over
5. Запуск таймера автопадения: `last_move_time = get_current_time_ms()`

**Код:**
```c
// Перемещаем следующую фигуру в текущую
state->current_figure = state->next_figure;

// Создаем новую следующую фигуру
state->next_figure = tetris_create_figure(tetris_get_random_figure_type());
state->stats.figures_count++;

// Проверяем коллизию при появлении (Game Over)
if (tetris_check_collision(&state->current_figure, state->public_info.field, 0, 0)) {
    state->fsm_state = STATE_GAME_OVER;
    return;
}
```

### 3. STATE_MOVING
**Файл:** `fsm.c:316-342`

Основное состояние игры. Обрабатывает:
- Пользовательский ввод (движение, поворот)
- Автоматическое падение по таймеру

**Автопадение (критическая логика):**
```c
if (is_time_to_move(state)) {
    if (!tetris_check_collision(&state->current_figure, state->public_info.field, 0, 1)) {
        // Можем двигать - двигаем
        state->current_figure.position.y++;
        state->last_move_time = get_current_time_ms();  // Сбрасываем таймер
    } else {
        // Не можем двигать - клеим фигуру
        state->fsm_state = STATE_ATTACHING;
    }
}
```

**Система времени:**
- Скорость падения зависит от уровня: `LEVEL_SPEEDS[level-1]`
- Уровень 1: 300ms, Уровень 10: 150ms
- Функция `is_time_to_move()` сравнивает `elapsed >= speed`

### 4. STATE_DROP
**Файл:** `fsm.c:171-192`

Принудительное падение при нажатии `Down`.

**Алгоритм мгновенного падения:**
```c
// Принудительное падение до упора
while (!tetris_check_collision(&state->current_figure, state->public_info.field, 0, 1)) {
    state->current_figure.position.y++;
}

// Переходим к прикреплению
state->fsm_state = STATE_ATTACHING;
```

### 5. STATE_ATTACHING (самое сложное состояние)
**Файл:** `fsm.c:194-271`

Обрабатывает анимацию очистки линий и прикрепление фигуры.

**Фазы выполнения:**

#### Фаза 1: Инициализация анимации
```c
if (state->animation_counter == 0) {
    // Временно размещаем фигуру для поиска линий
    tetris_place_figure_on_field(&state->current_figure, state->public_info.field);

    // Ищем заполненные линии
    state->animation_lines_count = tetris_find_full_lines(state->public_info.field, state->animation_lines);

    // Если нет линий - завершаем сразу
    if (state->animation_lines_count == 0) {
        // Проверяем Game Over или переходим к SPAWN
    }
}
```

#### Фаза 2: Анимация мигания
```c
if (state->animation_counter < state->animation_duration) {
    bool enable_blink = (state->animation_counter % 2 == 0);
    toggle_line_blinking(state->public_info.field, state->animation_lines,
                       state->animation_lines_count, enable_blink);
    state->animation_counter++;
    return;
}
```

**Система мигания:**
- Четные кадры: блоки +100 (мигание включено)
- Нечетные кадры: блоки -100 (мигание выключено)
- Длительность: 18 кадров (низкие уровни), 12 кадров (высокие)

#### Фаза 3: Завершение
```c
// Выключаем мигание
toggle_line_blinking(state->public_info.field, state->animation_lines,
                   state->animation_lines_count, false);

// Очищаем линии и начисляем очки
int lines_cleared = tetris_clear_lines(state->public_info.field);
if (lines_cleared > 0) {
    add_score_for_lines(state, lines_cleared);
}

// Проверяем Game Over
if (is_field_collision_at_top(state->public_info.field)) {
    state->fsm_state = STATE_GAME_OVER;
} else {
    state->fsm_state = STATE_SPAWN;
}
```

### 6. STATE_GAME_OVER
**Файл:** `fsm.c:278-295`

Финализация игры, установка pause=6 и обновление рекордов.

```c
static void fsm_state_game_over(TetrisState_t* state) {
    if (!state) return;

    // Устанавливаем pause = 6 для индикации game over
    state->public_info.pause = 6;

    // НОВОЕ: Окончательное обновление storage
    tetris_update_current_score();

    // Обновляем локальный high_score из storage на случай если есть новые рекорды
    int storage_high_score = tetris_sql_load_high_score();
    if (storage_high_score > state->public_info.high_score) {
        state->public_info.high_score = storage_high_score;
    }
}
```

**⚠️ КРИТИЧНО ДЛЯ ФРОНТА:** `pause = 6` означает что нужно показать game over screen!

## Система скоринга

### Функция add_score_for_lines()
**Файл:** `fsm.c:63-99`

**Система очков:**
```c
switch (lines_cleared) {
    case 1: points = POINTS_1_LINE;    // 100
    case 2: points = POINTS_2_LINES;   // 300
    case 3: points = POINTS_3_LINES;   // 700
    case 4: points = POINTS_4_LINES;   // 1500
}
```

**Алгоритм обновления:**
1. Вычисление очков за линии
2. **old_score = current_score** (для DEBUG)
3. Добавление очков: `score += points`
4. Обновление статистики: `lines_cleared += lines`
5. Синхронизация с storage: `tetris_sql_load_high_score()`
6. Обновление уровня: `update_level(state)`
7. Real-time сохранение: `tetris_update_current_score()`

**О переменной old_score:**
```c
int old_score = state->public_info.score;  // ❌ ПРОБЛЕМА: unused без DEBUG
state->public_info.score += points;

#ifdef DEBUG
printf("🔥 SCORE: +%d points for %d lines, total=%d (was %d)\n",
       points, lines_cleared, state->public_info.score, old_score);
#endif
```

**Назначение:** Только для отладочного вывода "было → стало". Не используется в игровой логике.

### Система уровней
**Файл:** `fsm.c:44-61`

```c
static void update_level(TetrisState_t* state) {
    int new_level = (state->public_info.score / POINTS_PER_LEVEL) + 1;  // 600 очков = уровень
    if (new_level > MAX_LEVEL) new_level = MAX_LEVEL;  // Максимум 10

    if (new_level != state->public_info.level) {
        state->public_info.level = new_level;
        state->public_info.speed = get_current_speed(state);
        tetris_update_current_score();  // Real-time обновление
    }
}
```

## Обработка пользовательского ввода

### Функция tetris_fsm_handle_input()
**Файл:** `fsm.c:360-479`

**Команды управления:**

#### Start/Pause
```c
if (state->fsm_state == STATE_START) {
    state->fsm_state = STATE_SPAWN;
} else if (state->fsm_state == STATE_GAME_OVER) {
    tetris_restart_game();
} else {
    state->public_info.pause = (state->public_info.pause == 1) ? 0 : 1;
}
```

#### Движение (Left/Right)
```c
if (state->fsm_state == STATE_MOVING && state->public_info.pause != 1) {
    if (!tetris_check_collision(&state->current_figure, field, offset_x, 0)) {
        state->current_figure.position.x += offset_x;
    }
}
```

#### Поворот (Up)
```c
Figure_t temp = state->current_figure;
if (tetris_rotate_figure(&temp, true)) {
    if (!tetris_check_collision(&temp, field, 0, 0)) {
        state->current_figure = temp;  // Применяем поворот
    }
}
```

#### Быстрое падение (Down)
```c
if (state->fsm_state == STATE_MOVING && state->public_info.pause != 1) {
    state->fsm_state = STATE_DROP;  // Мгновенный переход
}
```

## Система времени и скорости

### Функция is_time_to_move()
**Файл:** `fsm.c:27-42`

```c
static bool is_time_to_move(TetrisState_t* state) {
    long long current_time = get_current_time_ms();
    long long elapsed = current_time - state->last_move_time;
    long long speed = get_current_speed(state);

    return elapsed >= speed;
}
```

### Массив скоростей
```c
static const int LEVEL_SPEEDS[MAX_LEVEL] = {
    300, 850, 700, 600, 500, 400, 300, 250, 200, 150  // миллисекунды
};
```

## Интеграция с Storage

### Real-time обновления
FSM интегрирован с системой хранения для real-time обновлений:

- `tetris_update_current_score()` - при изменении уровня
- `tetris_sql_load_high_score()` - синхронизация рекорда
- `tetris_update_score()` - при Game Over

### Точки сохранения
1. **Изменение уровня** → `update_level()` → `tetris_update_current_score()`
2. **Очистка линий** → `add_score_for_lines()` → `tetris_update_current_score()`
3. **Game Over** → `fsm_state_game_over()` → `tetris_update_current_score()`
4. **Рестарт** → `tetris_restart_game()` → `tetris_update_score()`

## Отладка и диагностика

### DEBUG режим
Компилируется с флагом `-DDEBUG`:

```c
#ifdef DEBUG
printf("🕐 AUTO-FALL: elapsed=%lldms, moving down\n", elapsed);
printf("🆕 SPAWN: new figure spawned, timer started at %lld\n", time);
printf("🔥 ATTACHING: starting animation for %d lines\n", count);
#endif
```

### Типы DEBUG сообщений
- 🕐 Таймеры автопадения
- 🆕 Создание новых фигур
- 🔥 Анимация и очистка линий
- 🎮 Обработка пользовательского ввода
- 💨 Быстрое падение
- 💀 Game Over

## Производительность и оптимизация

### Критические моменты
1. **Анимация не блокирует игру** - выполняется покадрово
2. **Минимальные аллокации** - работа с существующими структурами
3. **Эффективная проверка времени** - простые арифметические операции
4. **Кэширование скорости** - пересчет только при смене уровня

### Потенциальные улучшения
1. Удалить unused переменную `old_score` при выключенном DEBUG
2. Оптимизировать частоту вызовов `get_current_time_ms()`
3. Кэшировать результаты `tetris_check_collision()` для повторных проверок