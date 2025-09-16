# STORAGE - Система хранения данных BrickGame

## Обзор архитектуры

Система хранения BrickGame предоставляет абстрактный интерфейс для сохранения игровых рекордов с поддержкой множественных backend'ов:
- **Основной режим:** SQLite3 база данных (производительность + features)
- **Fallback режим:** Файловое хранение (совместимость + надежность)
- **Абстрактный API:** Единый интерфейс для всех игр серии

## Архитектурная схема

```
┌─────────────────────────────────────────┐
│            Game Logic                    │
│        (tetris.c, fsm.c)                │
└─────────────────┬───────────────────────┘
                  │ tetris_storage_*()
                  │
┌─────────────────▼───────────────────────┐
│           Storage Abstraction            │
│             (storage.c)                 │
└─────────────────┬───────────────────────┘
                  │
      ┌───────────┴───────────┐
      │                       │
┌─────▼──────┐       ┌───────▼──────┐
│ SQL Storage│       │ File Storage │
│(sql_storage│       │(file_storage │
│    .c)     │       │     .c)      │
└────────────┘       └──────────────┘
```

## Абстрактный API

### Основные функции хранения

```c
/**
 * @brief Инициализация системы хранения
 * @return true при успехе, false при ошибке
 */
bool tetris_storage_init(void);

/**
 * @brief Загрузка лучшего результата
 * @return Максимальный счет или 0
 */
int tetris_load_high_score(void);

/**
 * @brief Обновление текущих результатов
 * @param current_score Текущий счет
 * @param lines_cleared Очищенных линий
 * @param level Достигнутый уровень
 * @return true при успехе
 */
bool tetris_update_score(int current_score, int lines_cleared, int level);

/**
 * @brief Очистка ресурсов хранения
 */
void tetris_storage_cleanup(void);
```

## Стратегия выбора backend'а

### Логика автоматического выбора
```c
// В storage.c
bool tetris_storage_init(void) {
    // Пытаемся инициализировать SQL storage
    if (tetris_sql_storage_init()) {
        current_storage_mode = STORAGE_MODE_SQL;
        return true;
    }

    // Fallback к файловому хранению
    if (tetris_file_storage_init()) {
        current_storage_mode = STORAGE_MODE_FILE;
        return true;
    }

    // Оба метода недоступны
    current_storage_mode = STORAGE_MODE_NONE;
    return false;
}
```

### Режимы работы
```c
typedef enum {
    STORAGE_MODE_NONE,     // Хранение отключено
    STORAGE_MODE_SQL,      // SQLite3 база данных
    STORAGE_MODE_FILE      // Файловое хранение
} StorageMode_t;
```

## SQL Storage Backend

### Структура базы данных

#### Таблица s21_brickgame_records
```sql
CREATE TABLE IF NOT EXISTS s21_brickgame_records (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    updated DATETIME DEFAULT CURRENT_TIMESTAMP,
    game TEXT NOT NULL,              -- 'tetris', 'snake', 'tanks'
    player TEXT NOT NULL,            -- 'new_player_N'
    score INTEGER NOT NULL,          -- игровой счет
    lines INTEGER NOT NULL DEFAULT 0, -- очищенных линий
    level INTEGER NOT NULL DEFAULT 1  -- достигнутый уровень
);
```

#### Индексы для производительности
```sql
CREATE INDEX IF NOT EXISTS idx_game_score ON s21_brickgame_records(game, score);
CREATE INDEX IF NOT EXISTS idx_updated ON s21_brickgame_records(updated);
```

### Система игроков

#### Автогенерация уникальных имен
```c
// Алгоритм создания игрока
int max_id = sql_get_max_id();
int next_id = max_id + 1;
snprintf(player_name, sizeof(player_name), "new_player_%d", next_id);
```

**Примеры имен:** `new_player_1`, `new_player_2`, `new_player_3`

#### Session management
```c
static int g_current_player_id = -1;  // ID текущей сессии

// При инициализации создается новый игрок
g_current_player_id = sql_create_player_record(player_name);

// Все обновления привязаны к этому ID
sql_update_player_record(g_current_player_id, score, lines, level);
```

### Real-time обновления

#### Точки сохранения
1. **Повышение уровня** → мгновенное сохранение
2. **Очистка линий** → мгновенное сохранение
3. **Game Over** → финальное сохранение
4. **Рестарт игры** → сохранение предыдущей сессии

#### Синхронизация high_score
```c
// После каждого обновления
int storage_high_score = tetris_sql_load_high_score();
if (storage_high_score > local_high_score) {
    local_high_score = storage_high_score;
}
```

## File Storage Backend

### Формат файла s21_brickgame_high_score.dat

#### Бинарная структура
```c
typedef struct {
    int magic_number;     // 0x42524B47 ("BRKG")
    int version;          // Версия формата файла
    int high_score;       // Лучший результат
    int reserved[13];     // Резерв для расширения
} FileStorageHeader_t;   // Всего 64 байта
```

#### Алгоритм чтения/записи
```c
bool file_storage_load_high_score(int* high_score) {
    FILE* file = fopen(FILE_PATH, "rb");
    if (!file) {
        *high_score = 0;
        return true;  // Первый запуск - это нормально
    }

    FileStorageHeader_t header;
    size_t read = fread(&header, sizeof(header), 1, file);
    fclose(file);

    if (read != 1 || header.magic_number != MAGIC_NUMBER) {
        *high_score = 0;  // Поврежденный файл
        return false;
    }

    *high_score = header.high_score;
    return true;
}
```

### Безопасность данных

#### Atomic записи
```c
// Запись во временный файл + атомарное перемещение
bool file_storage_save_high_score(int high_score) {
    char temp_path[512];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", FILE_PATH);

    FILE* file = fopen(temp_path, "wb");
    // ... запись данных ...
    fclose(file);

    // Атомарное перемещение
    return (rename(temp_path, FILE_PATH) == 0);
}
```

#### Проверка целостности
```c
// Magic number для обнаружения поврежденных файлов
#define MAGIC_NUMBER 0x42524B47  // "BRKG"

// CRC32 проверка (потенциальное улучшение)
uint32_t calculate_crc32(const void* data, size_t length);
```

## Graceful Degradation

### Обработка ошибок storage

#### Стратегия fallback'а
```c
int tetris_load_high_score(void) {
    switch (current_storage_mode) {
        case STORAGE_MODE_SQL:
            return tetris_sql_load_high_score();

        case STORAGE_MODE_FILE:
            int score = 0;
            file_storage_load_high_score(&score);
            return score;

        case STORAGE_MODE_NONE:
        default:
            return 0;  // Игра продолжается без сохранения
    }
}
```

#### Logging и диагностика
```c
#ifdef DEBUG
void storage_log_error(const char* operation, const char* error) {
    printf("💾 STORAGE ERROR [%s]: %s\n", operation, error);
}
#endif
```

## Производительность

### SQL оптимизации

#### Prepared statements
```c
// Кэшируем подготовленные запросы
static sqlite3_stmt* g_update_score_stmt = NULL;
static sqlite3_stmt* g_load_high_score_stmt = NULL;

bool sql_prepare_statements(void) {
    const char* update_sql =
        "UPDATE s21_brickgame_records "
        "SET score = ?, lines = ?, level = ?, updated = CURRENT_TIMESTAMP "
        "WHERE id = ?";

    return (sqlite3_prepare_v2(g_db, update_sql, -1, &g_update_score_stmt, NULL) == SQLITE_OK);
}
```

#### Транзакции для batch операций
```c
bool sql_batch_update_scores(ScoreUpdate_t* updates, int count) {
    sqlite3_exec(g_db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    for (int i = 0; i < count; i++) {
        if (!sql_update_single_score(&updates[i])) {
            sqlite3_exec(g_db, "ROLLBACK", NULL, NULL, NULL);
            return false;
        }
    }

    sqlite3_exec(g_db, "COMMIT", NULL, NULL, NULL);
    return true;
}
```

### File storage оптимизации

#### Memory mapping для больших файлов
```c
#ifdef USE_MMAP
#include <sys/mman.h>

typedef struct {
    void* mapped_data;
    size_t file_size;
    int fd;
} MappedFile_t;

MappedFile_t* file_storage_mmap(const char* path);
#endif
```

## Расширение для будущих игр

### Унифицированный формат записей

#### Структура GameRecord_t
```c
typedef struct {
    char game_name[32];        // "tetris", "snake", "tanks"
    char player_name[32];      // "new_player_N"
    int score;                 // Универсальный счет
    time_t timestamp;          // Время достижения

    // Игро-специфичные данные
    union {
        struct {               // Для Тетрис
            int lines_cleared;
            int level;
            int figures_count;
        } tetris;

        struct {               // Для Snake
            int length;
            int apples_eaten;
        } snake;

        struct {               // Для Tanks
            int enemies_killed;
            int waves_completed;
        } tanks;
    } game_data;
} GameRecord_t;
```

### API расширение

#### Универсальные функции
```c
bool storage_save_game_record(const GameRecord_t* record);
GameRecord_t* storage_load_top_records(const char* game_name, int limit);
bool storage_get_player_statistics(const char* player_name, PlayerStats_t* stats);
```

#### Экспорт и импорт
```c
bool storage_export_to_json(const char* filename);
bool storage_import_from_json(const char* filename);
bool storage_migrate_to_new_format(int target_version);
```

## Мониторинг и администрирование

### SQL Manager интеграция
```c
// Прямой доступ к базе данных для администрирования
bool storage_execute_admin_query(const char* sql,
                                 int (*callback)(void*, int, char**, char**),
                                 void* data);

// Статистики базы данных
typedef struct {
    int total_records;
    int unique_players;
    time_t oldest_record;
    time_t newest_record;
    size_t database_size;
} StorageStats_t;

StorageStats_t storage_get_statistics(void);
```

### Backup и восстановление
```c
bool storage_create_backup(const char* backup_path);
bool storage_restore_from_backup(const char* backup_path);
bool storage_verify_integrity(void);
```

## Безопасность

### Защита от SQL injection
- Все запросы используют prepared statements
- Валидация всех входных параметров
- Ограничения на длину строк

### Защита файлов
- Проверка magic numbers
- Валидация размеров полей
- Атомарные операции записи

### Контроль доступа
```c
bool storage_check_permissions(const char* path);
bool storage_secure_delete(const char* path);  // Безопасное удаление
```