# SQL_STORAGE.C - SQLite хранение рекордов Тетрис

## Обзор модуля

`brick_game/tetris/sql_storage.c` - единственный модуль хранения данных в проекте, реализующий:
- **SQLite3 базу данных** для хранения рекордов
- **Real-time обновления** счета во время игры
- **Автоматическое создание игроков** с уникальными именами
- **Глобальное управление** соединением с базой данных

## Глобальное состояние

### Статические переменные модуля
```c
static sqlite3 *g_db = NULL;           // Соединение с базой данных
static int g_current_player_id = -1;   // ID текущего игрока в базе
```

**Принципы управления:**
- **Singleton pattern** - одно соединение на всю программу
- **Session persistence** - ID игрока сохраняется до завершения
- **Lazy initialization** - база открывается по требованию

## Схема базы данных

### Таблица s21_brickgame_records
```sql
CREATE TABLE IF NOT EXISTS s21_brickgame_records (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    updated DATETIME DEFAULT CURRENT_TIMESTAMP,
    game TEXT NOT NULL,              -- 'tetris' (готовность к расширению)
    player TEXT NOT NULL,            -- 'new_player_N'
    score INTEGER NOT NULL,          -- текущий счет игры
    lines INTEGER NOT NULL DEFAULT 0, -- общее количество очищенных линий
    level INTEGER NOT NULL DEFAULT 1  -- достигнутый уровень (1-10)
);
```

**Архитектурные особенности:**
- **Автоинкремент ID** для уникальности записей
- **Timestamps** для отслеживания активности
- **Поле game** готово для будущих игр (Snake, Tanks)
- **Расширенная статистика** (lines, level) помимо score

## Константы и настройки

### Определения из tetris.h
```c
#define SQL_FILE "s21_brickgame_sql.db"
#define SQL_MAX_PLAYER_NAME_LENGTH 32
#define SQL_MAX_GAME_NAME_LENGTH 32
```

## Публичные функции (API)

### tetris_sql_create_table() - Создание таблицы
**Файл:** `sql_storage.c:117-141`

```c
bool tetris_sql_create_table(void) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS s21_brickgame_records ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "updated DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "game TEXT NOT NULL,"
        "player TEXT NOT NULL,"
        "score INTEGER NOT NULL,"
        "lines INTEGER NOT NULL DEFAULT 0,"
        "level INTEGER NOT NULL DEFAULT 1"
        ");";

    char *error_msg = NULL;
    int rc = sqlite3_exec(g_db, sql, NULL, NULL, &error_msg);

    if (rc != SQLITE_OK) {
        #ifdef DEBUG
        printf("💾 SQL: Error creating table: %s\n", error_msg);
        #endif
        sqlite3_free(error_msg);
        return false;
    }

    return true;
}
```

**Особенности:**
- `IF NOT EXISTS` - безопасное создание таблицы
- Прямое выполнение через `sqlite3_exec()`
- DEBUG вывод ошибок для отладки

### tetris_sql_storage_init() - Базовая инициализация
**Файл:** `sql_storage.c:147-163`

```c
bool tetris_sql_storage_init(void) {
    // Открываем базу данных
    int rc = sqlite3_open(SQL_FILE, &g_db);
    if (rc != SQLITE_OK) {
        return false;
    }

    // Создаем таблицу если нужно
    if (!tetris_sql_create_table()) {
        sqlite3_close(g_db);
        g_db = NULL;
        return false;
    }

    return true;
}
```

**Алгоритм:**
1. Открытие файла базы данных
2. Создание таблицы при необходимости
3. Откат при любой ошибке

**Используется:** В абстрактном слое storage.c

### tetris_sql_storage_init_and_create_player() - Полная инициализация
**Файл:** `sql_storage.c:169-190`

```c
bool tetris_sql_storage_init_and_create_player(void) {
    if (!tetris_sql_storage_init()) {
        return false;
    }

    // Получаем максимальный ID и создаем нового игрока
    int max_id = sql_get_max_id();
    int next_id = max_id + 1;

    char player_name[SQL_MAX_PLAYER_NAME_LENGTH];
    snprintf(player_name, sizeof(player_name), "new_player_%d", next_id);

    // Создаем запись для текущего игрока
    g_current_player_id = sql_create_player_record(player_name);

    if (g_current_player_id == -1) {
        tetris_sql_storage_cleanup();
        return false;
    }

    return true;
}
```

**Алгоритм создания игрока:**
1. Базовая инициализация базы данных
2. Поиск максимального ID: `SELECT MAX(id)`
3. Генерация уникального имени: `new_player_N`
4. Создание записи с начальными значениями
5. Сохранение ID для последующих обновлений

**Используется:** Потенциально в тестах или расширенных версиях

### tetris_sql_storage_cleanup() - Очистка ресурсов
**Файл:** `sql_storage.c:195-201`

```c
void tetris_sql_storage_cleanup(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
    g_current_player_id = -1;
}
```

**Безопасная очистка:**
- Проверка валидности соединения
- Сброс глобальных переменных
- Вызывается из `tetris_destroy()`

### tetris_sql_load_high_score() - Загрузка лучшего результата
**Файл:** `sql_storage.c:207-228`

```c
int tetris_sql_load_high_score(void) {
    if (!g_db) return 0;

    const char *sql = "SELECT MAX(score) FROM s21_brickgame_records WHERE game = 'tetris';";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return 0;

    int high_score = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        high_score = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return high_score;
}
```

**SQL логика:**
- `MAX(score)` - лучший результат среди всех игроков
- `WHERE game = 'tetris'` - фильтрация по типу игры
- `prepared statements` для безопасности и производительности

**Обработка ошибок:**
- Нет соединения → 0
- Ошибка prepare → 0
- Нет данных → 0 (первый запуск)

### tetris_sql_update_score() - Обновление счета
**Файл:** `sql_storage.c:237-250`

```c
bool tetris_sql_update_score(int current_score, int lines_cleared, int level) {
    if (!g_db || g_current_player_id == -1) {
        return false;
    }

    // Проверяем валидность данных
    if (current_score < 0 || current_score > 10000000 ||
        lines_cleared < 0 || level < 1 || level > 10) {
        return false;
    }

    // Обновляем запись текущего игрока
    return sql_update_player_record(g_current_player_id, current_score, lines_cleared, level);
}
```

**Валидация входных данных:**
- Счет: 0 ≤ score ≤ 10,000,000 (защита от overflow)
- Линии: lines ≥ 0 (не может быть отрицательным)
- Уровень: 1 ≤ level ≤ 10 (по спецификации игры)

**Требования для работы:**
- Активное соединение с базой данных
- Валидный ID текущего игрока

### tetris_sql_execute_query() - Выполнение произвольных запросов
**Файл:** `sql_storage.c:259-272`

```c
bool tetris_sql_execute_query(const char *sql, int (*callback)(void*, int, char**, char**), void *data) {
    if (!g_db) return false;

    char *error_msg = NULL;
    int rc = sqlite3_exec(g_db, sql, callback, data, &error_msg);

    if (error_msg) {
        sqlite3_free(error_msg);
    }

    return (rc == SQLITE_OK);
}
```

**Назначение:**
- Интерфейс для SQL Manager (`_sql_manager/`)
- Отладочные запросы и администрирование
- Экспорт данных и аналитика

## Статические функции (внутренняя логика)

### sql_get_max_id() - Получение максимального ID
**Файл:** `sql_storage.c:27-44`

```c
static int sql_get_max_id(void) {
    const char *sql = "SELECT MAX(id) FROM s21_brickgame_records;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return 0;

    int max_id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        max_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return max_id;
}
```

**Использование:**
- Генерация уникальных имен игроков
- Определение следующего доступного ID
- Возвращает 0 если таблица пустая

### sql_create_player_record() - Создание записи игрока
**Файл:** `sql_storage.c:51-74`

```c
static int sql_create_player_record(const char *player_name) {
    const char *sql =
        "INSERT INTO s21_brickgame_records (game, player, score, lines, level) "
        "VALUES ('tetris', ?, 0, 0, 1);";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;

    // Привязываем параметры
    sqlite3_bind_text(stmt, 1, player_name, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        return (int)sqlite3_last_insert_rowid(g_db);
    } else {
        return -1;
    }
}
```

**Начальные значения:**
- `game = 'tetris'` (фиксированная строка)
- `player = player_name` (параметр)
- `score = 0` (начальный счет)
- `lines = 0` (линии не очищены)
- `level = 1` (стартовый уровень)

**Возвращаемое значение:**
- При успехе: ID новой записи (`sqlite3_last_insert_rowid`)
- При ошибке: -1

### sql_update_player_record() - Обновление записи игрока
**Файл:** `sql_storage.c:84-107`

```c
static bool sql_update_player_record(int player_id, int score, int lines, int level) {
    const char *sql =
        "UPDATE s21_brickgame_records "
        "SET updated = CURRENT_TIMESTAMP, score = ?, lines = ?, level = ? "
        "WHERE id = ?;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return false;

    // Привязываем параметры
    sqlite3_bind_int(stmt, 1, score);
    sqlite3_bind_int(stmt, 2, lines);
    sqlite3_bind_int(stmt, 3, level);
    sqlite3_bind_int(stmt, 4, player_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE);
}
```

**Обновляемые поля:**
- `updated = CURRENT_TIMESTAMP` (автоматический timestamp)
- `score = ?` (новый счет)
- `lines = ?` (общее количество линий)
- `level = ?` (достигнутый уровень)

**SQL оптимизации:**
- Prepared statement с параметрами
- Точное обновление по ID
- Эффективное использование индекса PRIMARY KEY

## Интеграция с игровой логикой

### Вызовы из tetris.c

#### 1. Инициализация (tetris_init)
```c
// НОВОЕ: Инициализируем систему хранения
if (!tetris_storage_init()) {
    #ifdef DEBUG
    printf("❌ INIT: Failed to initialize storage system\n");
    #endif
    return false;
}

// Загружаем рекорд из storage
g_tetris_state.public_info.high_score = tetris_sql_load_high_score();
```

#### 2. Real-time обновления (tetris_update_current_score)
```c
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
```

#### 3. Финализация (tetris_destroy, tetris_restart_game)
```c
// НОВОЕ: Финальное обновление счета в хранилище
tetris_update_score(g_tetris_state.public_info.score,
                   g_tetris_state.stats.lines_cleared,
                   g_tetris_state.public_info.level);

// НОВОЕ: Очищаем систему хранения
tetris_storage_cleanup();
```

### Вызовы из FSM (fsm.c)

#### 1. Обновление при изменении уровня (update_level)
```c
if (new_level != state->public_info.level) {
    state->public_info.level = new_level;
    state->public_info.speed = get_current_speed(state);

    #ifdef DEBUG
    printf("📈 LEVEL UP: New level %d, speed %d\n", new_level, state->public_info.speed);
    #endif

    // НОВОЕ: Обновляем storage при изменении уровня
    tetris_update_current_score();
}
```

#### 2. Обновление при очистке линий (add_score_for_lines)
```c
state->public_info.score += points;
state->stats.lines_cleared += lines_cleared;

// Обновляем high_score из storage
int storage_high_score = tetris_sql_load_high_score();
if (storage_high_score > state->public_info.high_score) {
    state->public_info.high_score = storage_high_score;
}

update_level(state);

// НОВОЕ: Обновляем storage при изменении счета
tetris_update_current_score();
```

#### 3. Финальное обновление при Game Over (fsm_state_game_over)
```c
// НОВОЕ: Окончательное обновление storage
tetris_update_current_score();

// Обновляем локальный high_score из storage на случай если есть новые рекорды
int storage_high_score = tetris_sql_load_high_score();
if (storage_high_score > state->public_info.high_score) {
    state->public_info.high_score = storage_high_score;
}
```

## Real-time система обновлений

### Философия real-time storage
Система спроектирована для **непрерывного сохранения** прогресса:

1. **При повышении уровня** → мгновенное сохранение
2. **При очистке линий** → мгновенное сохранение
3. **При Game Over** → финальное сохранение
4. **При рестарте** → сохранение предыдущей сессии

### Преимущества подхода
- **Нет потери данных** при внезапном завершении
- **Актуальные рекорды** между сессиями
- **Мгновенная синхронизация** high_score

### Производительность
- **Prepared statements** - быстрые SQL операции
- **Минимальные данные** - только измененные поля
- **Эффективные индексы** - PRIMARY KEY для быстрого поиска

## Система именования игроков

### Алгоритм генерации имен
```c
int max_id = sql_get_max_id();           // Максимальный ID из базы
int next_id = max_id + 1;                // Следующий доступный ID
snprintf(player_name, sizeof(player_name), "new_player_%d", next_id);
```

**Примеры имен:**
- Первый игрок: `new_player_1`
- Второй игрок: `new_player_2`
- После удаления записи 2: `new_player_3` (не переиспользует)

### Преимущества подхода
- **Уникальность гарантирована** через автоинкремент
- **Простота реализации**
- **Читаемость** для отладки
- **Расширяемость** (легко изменить формат)

## Обработка ошибок и надежность

### Graceful degradation
Все функции спроектированы для **мягкого отказа**:

```c
// Примеры безопасного поведения
int tetris_sql_load_high_score(void) {
    if (!g_db) return 0;              // Нет соединения → 0
    // ... запрос к базе ...
    if (rc != SQLITE_OK) return 0;    // Ошибка SQL → 0
}

bool tetris_sql_update_score(...) {
    if (!g_db || g_current_player_id == -1) {
        return false;                 // Нет инициализации → false
    }
    // ... валидация данных ...
    if (invalid_data) return false;   // Плохие данные → false
}
```

### Стратегии восстановления
1. **При ошибке загрузки** → high_score = 0 (игра продолжается)
2. **При ошибке сохранения** → игра продолжается (данные в памяти)
3. **При ошибке инициализации** → возможен fallback к file storage

### Валидация данных
Строгие проверки предотвращают corruption базы данных:

```c
// Проверки диапазонов
if (current_score < 0 || current_score > 10000000 ||
    lines_cleared < 0 || level < 1 || level > 10) {
    return false;
}
```

## Отладка и мониторинг

### DEBUG вывод
```c
#ifdef DEBUG
printf("💾 SQL: Error creating table: %s\n", error_msg);
printf("🏆 GAME_OVER: Updated high score from storage: %d\n", storage_high_score);
printf("🏆 NEW HIGH SCORE: %d\n", current_high);
#endif
```

### SQL Manager интеграция
Функция `tetris_sql_execute_query()` позволяет:
- Прямые SQL запросы к базе данных
- Экспорт всех записей для анализа
- Администрирование базы данных

## Архитектурные принципы

### Инкапсуляция
- Все SQLite детали скрыты в модуле
- Публичный API через простые функции
- Статические функции для внутренней логики

### Единственность соединения
- Одно глобальное соединение `g_db`
- Инициализация по требованию
- Автоматическая очистка ресурсов

### Готовность к расширению
- Поле `game` готово для новых игр
- Расширенная статистика (lines, level)
- Модульная структура функций

### Безопасность
- Prepared statements против SQL injection
- Валидация всех входных данных
- Graceful degradation при ошибках