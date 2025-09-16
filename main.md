# MAIN.C - Главный файл CLI интерфейса

## Обзор модуля

`gui/cli/main.c` - точка входа и основной цикл CLI интерфейса с поддержкой динамической загрузки игровых библиотек:
- **Сканирование и выбор** игровых библиотек (.so файлов)
- **Динамическая загрузка** выбранной библиотеки игры
- **Управление состояниями CLI** (выбор библиотек, активная игра)
- **Главный игровой цикл** с обработкой времени
- **Строгое соответствие спецификации** - использует только userInput/updateCurrentState API

## Архитектура главного цикла

### Общая структура программы
```
main()
├── initialize_system()      // Настройка терминала и signal handlers
├── library_scanning()       // Поиск доступных .so библиотек
├── game_loop()              // Основной цикл до завершения
│   ├── input_processing()   // Обработка пользовательского ввода
│   ├── library_management() // Переключение между библиотеками
│   ├── game_state_update()  // Вызов updateCurrentState()
│   ├── display_update()     // Отрисовка по состоянию pause
│   └── timing_control()     // Контроль частоты кадров (20 FPS)
└── cleanup_system()         // Финальная очистка ресурсов
```

## Архитектура состояний CLI

### CLI состояния (реальная реализация)
```c
typedef enum {
    CLI_STATE_LIBRARY_SELECTION,    // Выбор библиотеки игры
    CLI_STATE_GAME_ACTIVE           // Игра активна (библиотека управляет состоянием)
} CliState_t;
```

### Переходы состояний
```
Запуск программы
    ↓
[Сканирование .so файлов через scanner_scan_libraries()]
    ↓
CLI_STATE_LIBRARY_SELECTION ←→ CLI_STATE_GAME_ACTIVE
    ↓ (Enter)                        ↓ (L key)
[load_game_library()]               [Возврат к выбору + ресканирование]
    ↓
CLI_STATE_GAME_ACTIVE
    ↓ (Q/Esc)
[Завершение]
```

### Логика выбора библиотек (актуальная реализация)
**Файл:** `main.c:126-142`

```c
// Сканируем доступные библиотеки
available_libraries = scanner_scan_libraries(".");

// Показываем экран выбора библиотек или загружаем по умолчанию
if (available_libraries && available_libraries->count > 0) {
    display_draw_library_selection_screen(available_libraries->libraries,
                                          available_libraries->count,
                                          available_libraries->selected_index);
} else {
    // Пытаемся загрузить библиотеку по умолчанию
    if (load_game_library(DEFAULT_LIB_PATH, "Tetris")) {
        cli_state = CLI_STATE_GAME_ACTIVE;
        // НЕ показываем экран здесь - пусть main loop определит что показывать по pause
    } else {
        display_draw_library_selection_screen(NULL, 0, 0);
    }
}
```

## Константы и настройки

### Параметры производительности
**Файл:** `main.c:18-19`

```c
#define REFRESH_RATE_MS 50      // 20 FPS для отрисовки
#define DEFAULT_LIB_PATH "./libtetris.so"
```

**Обоснование частоты кадров:**
- **50ms = 20 FPS** - достаточно для плавной отрисовки текста
- **Экономия CPU** - не нагружаем систему излишними перерисовками
- **Отзывчивость** - пользователь не замечает задержек

## Глобальное состояние

### Статические переменные
**Файл:** `main.c:25-26`

```c
static volatile bool running = true;
static TetrisLibrary_t* tetris_lib = NULL;
```

**Принципы дизайна:**
- **`volatile bool running`** - корректная работа с signal handlers
- **`static`** - инкапсуляция в модуле main.c
- **Глобальный доступ** - необходим для signal handlers и основного цикла

## Обработка сигналов

### signal_handler() - Корректное завершение
**Файл:** `main.c:33-36`

```c
void signal_handler(int signum) {
    (void)signum;  // Подавляем предупреждение о неиспользуемом параметре
    running = false;
}
```

### Установка обработчиков
**Из initialize_system():**
```c
signal(SIGINT, signal_handler);   // Ctrl+C
signal(SIGTERM, signal_handler);  // Команда завершения системы
```

**Важность signal handling:**
- **Graceful shutdown** - корректное восстановление терминала
- **Предотвращение зависших терминалов** при аварийном завершении
- **Сохранение данных** через cleanup процедуры

### Безопасность в signal handlers
```c
// ✅ Безопасно - простое присвоение atomic переменной
running = false;

// ❌ Небезопасно в signal handler
printf("Terminating...\n");
library_unload(tetris_lib);
```

## Инициализация системы

### initialize_system() - Полная инициализация
**Файл:** `main.c:42-66`

```c
bool initialize_system(void) {
    // Установка обработчиков сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Загрузка библиотеки
    tetris_lib = library_load(DEFAULT_LIB_PATH);
    if (!tetris_lib) {
        fprintf(stderr, "Не удалось загрузить библиотеку Tetris\n");
        return false;
    }

    // Инициализация библиотеки
    if (!library_init_game(tetris_lib)) {
        fprintf(stderr, "Не удалось инициализировать игру\n");
        library_unload(tetris_lib);
        return false;
    }

    // Настройка терминала
    terminal_setup();
    terminal_hide_cursor();

    return true;
}
```

### Последовательность инициализации

#### 1. Signal handlers
```c
signal(SIGINT, signal_handler);
signal(SIGTERM, signal_handler);
```
**Приоритет:** Устанавливается в первую очередь для безопасности.

#### 2. Загрузка игровой библиотеки
```c
tetris_lib = library_load(DEFAULT_LIB_PATH);
```
**Проверка:** Возврат NULL при ошибке загрузки или отсутствии файла.

#### 3. Инициализация игровой логики
```c
if (!library_init_game(tetris_lib)) {
    library_unload(tetris_lib);  // Откат при ошибке
    return false;
}
```

#### 4. Настройка терминала
```c
terminal_setup();      // Неблокирующий ввод
terminal_hide_cursor(); // Скрытие курсора для чистого интерфейса
```

### Обработка ошибок инициализации
При любой ошибке происходит откат:
- Выгрузка уже загруженной библиотеки
- Восстановление терминала (в cleanup_system)
- Возврат `false` для завершения программы

## Очистка ресурсов

### cleanup_system() - Финальная очистка
**Файл:** `main.c:68-77`

```c
void cleanup_system(void) {
    // Восстановление терминала
    terminal_restore();

    // Деинициализация игры и выгрузка библиотеки
    if (tetris_lib) {
        library_destroy_game(tetris_lib);
        library_unload(tetris_lib);
    }
}
```

### Порядок очистки

#### 1. Восстановление терминала
```c
terminal_restore();
```
**Критически важно:** Восстанавливает исходные настройки терминала, показывает курсор.

#### 2. Деинициализация игры
```c
library_destroy_game(tetris_lib);
```
**Эффект:** Сохранение финального состояния, освобождение ресурсов игры.

#### 3. Выгрузка библиотеки
```c
library_unload(tetris_lib);
```
**Эффект:** Освобождение памяти интерфейса, выгрузка .so файла.

## Состояния GUI

### CliGameState_t - Перечисление состояний
**Файл:** `main.c:83-87`

```c
typedef enum {
    GAME_STATE_START_SCREEN,
    GAME_STATE_PLAYING,
    GAME_STATE_GAME_OVER
} CliGameState_t;
```

### Диаграмма переходов состояний
```
START_SCREEN ──Start──▶ PLAYING ──GameOver──▶ GAME_OVER
      ▲                     │                      │
      │                     │                      │
      └─────────────────────┴──────Start───────────┘
```

**Переходы:**
- **START_SCREEN → PLAYING:** Пользователь нажимает Start/Enter
- **PLAYING → GAME_OVER:** Игра заканчивается (столкновение в верхней части)
- **GAME_OVER → PLAYING:** Пользователь нажимает Start для рестарта
- **ЛЮБОЕ → EXIT:** Пользователь нажимает Q/Esc

## Главный игровой цикл

### main() - Точка входа программы
**Файл:** `main.c:93-191`

```c
int main(void) {
    // Инициализация системы
    if (!initialize_system()) {
        return 1;
    }

    // Переменные игрового цикла
    struct timeval last_update, current_time;
    gettimeofday(&last_update, NULL);

    CliGameState_t cli_state = GAME_STATE_START_SCREEN;
    bool game_initialized = false;

    // Показываем стартовый экран
    display_draw_start_screen();

    // Главный цикл программы
    while (running) {
        gettimeofday(&current_time, NULL);

        // Вычисляем разность времени в миллисекундах
        long elapsed_ms = (current_time.tv_sec - last_update.tv_sec) * 1000 +
                         (current_time.tv_usec - last_update.tv_usec) / 1000;

        // Обработка пользовательского ввода
        int key = input_get_key();
        if (key != -1) {
            // ... обработка ввода ...
        }

        // Обновление отображения с заданной частотой
        if (elapsed_ms >= REFRESH_RATE_MS) {
            // ... обновление экрана ...
            last_update = current_time;
        }

        // Небольшая пауза для снижения нагрузки на CPU
        usleep(10000);  // 10ms
    }

    // Очистка и завершение
    cleanup_system();
    printf("\nСпасибо за игру в BrickGame Tetris!\n");
    return 0;
}
```

## Обработка пользовательского ввода

### Алгоритм обработки клавиш по состояниям CLI
**Файл:** `main.c:152-214`

#### CLI_STATE_LIBRARY_SELECTION - Навигация по библиотекам
```c
if (cli_state == CLI_STATE_LIBRARY_SELECTION) {
    if (key == KEY_ARROW_UP || key == 'w' || key == 'W') {
        scanner_navigate_selection(available_libraries, -1);  // Вверх
    } else if (key == KEY_ARROW_DOWN) {
        scanner_navigate_selection(available_libraries, 1);   // Вниз
    } else if (key == KEY_ENTER || key == KEY_CARRIAGE) {
        // Загружаем выбранную библиотеку
        load_game_library(lib_path, game_name);
        cli_state = CLI_STATE_GAME_ACTIVE;
    }
}
```

#### CLI_STATE_GAME_ACTIVE - Игровой ввод
```c
else if (cli_state == CLI_STATE_GAME_ACTIVE && tetris_lib) {
    if (key == 'l' || key == 'L') {
        // Возврат к выбору библиотек
        cli_state = CLI_STATE_LIBRARY_SELECTION;
    } else {
        // СТРОГО ПО СПЕЦИФИКАЦИИ - только userInput!
        UserAction_t action = input_key_to_action(key);
        if (action != -1) {
            tetris_lib->userInput(action, false);
            // game_initialized управляется состоянием библиотеки через pause
        }
    }
}
                }
            }
        }
    }
}
```

### Приоритет обработки

#### 1. Команды выхода (максимальный приоритет)
```c
if (input_is_quit_key(key)) {
    running = false;
    continue;  // Пропускаем остальную обработку
}
```
**Клавиши:** Q, q, Esc

#### 2. Игровые действия
```c
UserAction_t action = input_key_to_action(key);
if (action != -1) {
    tetris_lib->userInput(action, false);
}
```
**Эффект:** Передача всех игровых команд в библиотеку.

## ⚠️ КРИТИЧЕСКОЕ: Определение состояний игры через pause (СТРОГО ПО СПЕЦИФИКАЦИИ!)

### Логика отображения по pause - АКТУАЛЬНАЯ РЕАЛИЗАЦИЯ
**Файл:** `main.c:222-238`

```c
// Получаем состояние игры от библиотеки
GameInfo_t game_info = tetris_lib->updateCurrentState();

// Проверяем состояние игры по полю pause
if (game_info.pause == 6) {
    // Game over
    display_draw_universal_game_over_screen(current_game_name, game_info.score, game_info.high_score);
} else if (game_info.pause == 7) {
    // Start screen - сбрасываем флаг инициализации
    game_initialized = false;
    display_draw_universal_start_screen(current_game_name);
} else {
    // Игровой процесс - устанавливаем флаг инициализации
    game_initialized = true;
    display_draw_field(&game_info, false, true);
}
```

**СТРОГО ПО СПЕЦИФИКАЦИИ:** Используется ТОЛЬКО API `updateCurrentState()`, состояние определяется ТОЛЬКО через поле `pause`.

**Значения pause и их интерпретация:**
- **pause = 0** - обычная игра (показать игровое поле)
- **pause = 1** - игра на паузе (показать игровое поле с pause overlay)
- **pause = 6** - ⚠️ GAME OVER (показать game over экран)
- **pause = 7** - ⚠️ START SCREEN (показать start экран)

**Синхронизация game_initialized:**
- `pause = 7` → `game_initialized = false` (показываем start screen)
- `pause = 6` → показываем game over (game_initialized не важен)
- Любое другое → `game_initialized = true` (показываем игровое поле)

## Управление отображением

### Система обновления кадров
**Файл:** `main.c:222-241`

```c
// Обновление отображения с заданной частотой
if (elapsed_ms >= REFRESH_RATE_MS) {
    switch (cli_state) {
        case GAME_STATE_START_SCREEN:
            // Стартовый экран уже показан, ждем ввода
            break;

        case GAME_STATE_PLAYING: {
            // Получаем состояние игры
            GameInfo_t game_info = tetris_lib->updateCurrentState();

            // Состояние определяется ТОЛЬКО через pause поле (по спецификации)

            // Используем pause поле для определения что показывать
            if (game_info.pause == 6) {
                // Game over
                display_draw_universal_game_over_screen(current_game_name, game_info.score, game_info.high_score);
            } else if (game_info.pause == 7) {
                // Start screen
                display_draw_universal_start_screen(current_game_name);
            } else {
                // Игровое поле
                display_draw_field(&game_info, false, true);
            }
            break;
        }

        case GAME_STATE_GAME_OVER:
            // Экран game over уже показан, ждем ввода
            break;
    }

    last_update = current_time;
}
```

### Логика состояний отображения

#### GAME_STATE_START_SCREEN
```c
// Экран показан один раз в начале программы
display_draw_start_screen();  // Вызывается перед циклом

// В цикле - ничего не делаем
case GAME_STATE_START_SCREEN:
    break;
```

#### GAME_STATE_PLAYING
```c
// Получаем актуальное состояние от библиотеки
GameInfo_t game_info = tetris_lib->updateCurrentState();

// Состояние определяется через pause поле (спецификация)
if (game_info.pause == 6) {
    // Game over
    display_draw_universal_game_over_screen(current_game_name, game_info.score, game_info.high_score);
} else if (game_info.pause == 7) {
    // Start screen
    display_draw_universal_start_screen(current_game_name);
} else {
    // Игровое поле
    display_draw_field(&game_info, false, true);
}
```

#### GAME_STATE_GAME_OVER
```c
// Экран показан при переходе из PLAYING
// В цикле - ничего не делаем, ждем пользовательского ввода
case GAME_STATE_GAME_OVER:
    break;
```

## Система тайминга

### Высокоточное измерение времени
```c
struct timeval last_update, current_time;
gettimeofday(&last_update, NULL);

// В цикле
gettimeofday(&current_time, NULL);
long elapsed_ms = (current_time.tv_sec - last_update.tv_sec) * 1000 +
                 (current_time.tv_usec - last_update.tv_usec) / 1000;
```

**Точность:** Микросекунды (`tv_usec`), достаточно для плавной анимации.

### Контроль частоты кадров
```c
if (elapsed_ms >= REFRESH_RATE_MS) {
    // Обновление экрана
    last_update = current_time;
}

// Пауза для снижения нагрузки на CPU
usleep(10000);  // 10ms
```

**Стратегия:**
- **Проверка каждую итерацию** - максимальная отзывчивость ввода
- **Обновление по таймеру** - контролируемая частота кадров
- **usleep(10ms)** - предотвращение 100% загрузки CPU

## Архитектурные принципы

### Разделение ответственности
- **main.c** - игровой цикл, состояния GUI, тайминг
- **library.c** - загрузка и интерфейс к игровой логике
- **display.c** - отрисовка всех экранов
- **input.c** - обработка клавиатуры
- **terminal.c** - управление терминалом

### Минимальная связанность
```c
// main.c не знает детали реализации других модулей
tetris_lib->userInput(action, false);     // Абстракция библиотеки
display_draw_field(&game_info, ...);      // Абстракция отрисовки
input_get_key();                          // Абстракция ввода
```

### Обработка ошибок
- **Проверка NULL** для всех указателей на функции библиотеки
- **Откат инициализации** при любой ошибке
- **Graceful degradation** при отсутствии дополнительных функций

## Производительность

### CPU использование
```c
while (running) {
    // Быстрые операции:
    input_get_key();          // O(1) - getchar()
    time_calculation();       // O(1) - арифметика

    // Тяжелые операции (20 FPS):
    updateCurrentState();     // Зависит от библиотеки
    display_draw_field();     // ~1000 printf() вызовов

    usleep(10000);           // 10ms пауза
}
```

**Примерная нагрузка:**
- **90% времени** - usleep() (idle)
- **5% времени** - отрисовка
- **5% времени** - логика игры и ввод

### Оптимизации
1. **Ленивая отрисовка** - обновление только при изменениях
2. **Буферизация ввода** - обработка множественных клавиш за кадр
3. **Переменная частота кадров** - адаптация к производительности системы

## Расширение функциональности

### Добавление новых состояний
```c
typedef enum {
    GAME_STATE_START_SCREEN,
    GAME_STATE_PLAYING,
    GAME_STATE_PAUSED,        // Новое состояние
    GAME_STATE_SETTINGS,      // Новое состояние
    GAME_STATE_GAME_OVER
} CliGameState_t;
```

### Система конфигурации
```c
typedef struct {
    int refresh_rate_ms;
    char lib_path[256];
    bool enable_sound;
} Config_t;

Config_t load_config(const char* config_file);
```

### Поддержка звука
```c
// Интеграция аудио библиотеки
void play_sound_effect(SoundType_t sound);
// Вызовы в игровом цикле при определенных событиях
```

### Система достижений
```c
typedef struct {
    int total_lines_cleared;
    int games_played;
    int best_score;
    time_t total_play_time;
} Statistics_t;
```

### Мультиплеер поддержка
```c
// Сетевой режим с синхронизацией состояний
bool network_mode_enabled;
void sync_game_state_with_peer(GameInfo_t* game_info);
```