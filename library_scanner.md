# LIBRARY_SCANNER - Сканирование и выбор игровых библиотек

## Обзор модуля

`gui/cli/library_scanner.c` реализует систему автоматического обнаружения и выбора игровых библиотек:
- **Автоматическое сканирование** .so файлов в директории
- **Интерактивная навигация** по списку доступных библиотек
- **Управление выбором** с клавиатуры (↑↓ + Enter)
- **Динамическое управление памятью** для списков библиотек

## Структуры данных

### LibraryInfo_t - Информация о библиотеке
```c
typedef struct {
    char name[256];       // Имя библиотеки (без .so)
    char path[512];       // Полный путь к .so файлу
} LibraryInfo_t;
```

### LibraryList_t - Список библиотек
```c
typedef struct {
    LibraryInfo_t* libraries;    // Массив библиотек
    int count;                   // Количество найденных библиотек
    int selected_index;          // Индекс выбранной библиотеки
} LibraryList_t;
```

## Основные функции

### scanner_scan_libraries() - Сканирование директории
**Файл:** `library_scanner.c`

```c
LibraryList_t* scanner_scan_libraries(const char* directory) {
    // Открываем директорию
    DIR* dir = opendir(directory);
    if (!dir) return NULL;

    // Создаем список библиотек
    LibraryList_t* list = calloc(1, sizeof(LibraryList_t));

    // Сканируем файлы с расширением .so
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (is_library_file(entry->d_name)) {
            add_library_to_list(list, entry->d_name, directory);
        }
    }

    closedir(dir);
    return list;
}
```

**Алгоритм:**
1. Открывает указанную директорию
2. Перебирает все файлы в поиске .so библиотек
3. Для каждой найденной библиотеки создает LibraryInfo_t
4. Возвращает LibraryList_t с результатами

### scanner_navigate_selection() - Навигация по списку
```c
void scanner_navigate_selection(LibraryList_t* list, int direction) {
    if (!list || list->count == 0) return;

    list->selected_index += direction;

    // Циклическая навигация
    if (list->selected_index < 0) {
        list->selected_index = list->count - 1;
    } else if (list->selected_index >= list->count) {
        list->selected_index = 0;
    }
}
```

**Параметры:**
- `direction = -1` - вверх по списку
- `direction = +1` - вниз по списку
- Циклическая навигация (выход за границы возвращает на противоположный конец)

### scanner_get_selected_library_path() - Получение пути выбранной библиотеки
```c
const char* scanner_get_selected_library_path(LibraryList_t* list) {
    if (!list || list->count == 0 ||
        list->selected_index < 0 || list->selected_index >= list->count) {
        return NULL;
    }

    return list->libraries[list->selected_index].path;
}
```

### scanner_get_selected_library_name() - Получение имени выбранной библиотеки
```c
const char* scanner_get_selected_library_name(LibraryList_t* list) {
    if (!list || list->count == 0 ||
        list->selected_index < 0 || list->selected_index >= list->count) {
        return NULL;
    }

    return list->libraries[list->selected_index].name;
}
```

### scanner_free_library_list() - Освобождение памяти
```c
void scanner_free_library_list(LibraryList_t* list) {
    if (!list) return;

    if (list->libraries) {
        free(list->libraries);
    }
    free(list);
}
```

## Вспомогательные функции

### is_library_file() - Проверка расширения файла
```c
static bool is_library_file(const char* filename) {
    size_t len = strlen(filename);
    return (len > 3 && strcmp(filename + len - 3, ".so") == 0);
}
```

### add_library_to_list() - Добавление библиотеки в список
```c
static void add_library_to_list(LibraryList_t* list, const char* filename, const char* directory) {
    // Реаллокация массива библиотек
    list->libraries = realloc(list->libraries, (list->count + 1) * sizeof(LibraryInfo_t));

    // Создание полного пути
    snprintf(list->libraries[list->count].path, sizeof(list->libraries[list->count].path),
             "%s/%s", directory, filename);

    // Извлечение имени без расширения
    extract_library_name(filename, list->libraries[list->count].name);

    list->count++;
}
```

### extract_library_name() - Извлечение имени библиотеки
```c
static void extract_library_name(const char* filename, char* name) {
    // Копируем имя без расширения .so
    size_t len = strlen(filename);
    if (len > 3) {
        strncpy(name, filename, len - 3);
        name[len - 3] = '\0';
    } else {
        strcpy(name, filename);
    }

    // Убираем префикс "lib" если есть
    if (strncmp(name, "lib", 3) == 0) {
        memmove(name, name + 3, strlen(name + 3) + 1);
    }
}
```

**Примеры преобразования:**
- `libtetris.so` → `tetris`
- `libbrick_snake.so` → `brick_snake`
- `tetris.so` → `tetris`

## Интеграция с main.c

### Использование в главном цикле
```c
// Сканирование при запуске
LibraryList_t* available_libraries = scanner_scan_libraries(".");

// Отображение списка
if (available_libraries && available_libraries->count > 0) {
    display_draw_library_selection_screen(available_libraries->libraries,
                                          available_libraries->count,
                                          available_libraries->selected_index);
}

// Навигация в игровом цикле
if (key == KEY_ARROW_UP) {
    scanner_navigate_selection(available_libraries, -1);
} else if (key == KEY_ARROW_DOWN) {
    scanner_navigate_selection(available_libraries, 1);
} else if (key == KEY_ENTER) {
    const char* lib_path = scanner_get_selected_library_path(available_libraries);
    const char* game_name = scanner_get_selected_library_name(available_libraries);
    load_game_library(lib_path, game_name);
}

// Очистка при завершении
scanner_free_library_list(available_libraries);
```

## Архитектурные принципы

### Расширяемость
- Готовность к поддержке множественных игр в серии BrickGame
- Автоматическое обнаружение новых библиотек без изменения кода
- Масштабируемая система навигации

### Безопасность памяти
- Корректное управление динамической памятью
- Проверки границ массивов при навигации
- Graceful handling null указателей

### Производительность
- Сканирование только при необходимости (startup, rescanning)
- Кэширование результатов сканирования
- Эффективная realloc стратегия для роста списков

## Потенциальные улучшения

### Кэширование и производительность
```c
typedef struct {
    LibraryList_t* cached_list;
    time_t last_scan_time;
    char last_scan_directory[256];
} ScanCache_t;
```

### Фильтрация и сортировка
```c
typedef enum {
    SORT_BY_NAME,
    SORT_BY_DATE,
    SORT_BY_SIZE
} SortMode_t;

void scanner_sort_libraries(LibraryList_t* list, SortMode_t mode);
```

### Метаданные библиотек
```c
typedef struct {
    char version[32];
    char description[256];
    char author[64];
    time_t build_time;
} LibraryMetadata_t;

LibraryMetadata_t scanner_get_library_metadata(const char* lib_path);
```

### Асинхронное сканирование
```c
typedef struct {
    pthread_t scan_thread;
    volatile bool scan_complete;
    LibraryList_t* result;
} AsyncScan_t;

AsyncScan_t* scanner_scan_libraries_async(const char* directory);
```