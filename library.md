# LIBRARY.H/C - Динамическая загрузка библиотеки Tetris

## Обзор модуля

`gui/cli/library.c` реализует систему динамической загрузки игровой библиотеки:
- **Загрузка .so/.dylib файлов** через dlopen/dlsym
- **Безопасное получение символов** функций с проверкой
- **Абстракция API** библиотеки через структуру указателей
- **Graceful degradation** при отсутствии необязательных функций

## Архитектурная концепция

### Разделение GUI и игровой логики
```
┌─────────────────┐     ┌──────────────────┐
│ GUI (main.c)    │────▶│ library.c        │
│ - Отрисовка     │     │ - Загрузка .so   │
│ - Ввод          │     │ - Маппинг функций│
│ - Игровой цикл  │     └──────────────────┘
└─────────────────┘              │
                                 ▼
                     ┌──────────────────┐
                     │ libtetris.so     │
                     │ - userInput()    │
                     │ - updateState()  │
                     │ - tetris_init()  │
                     └──────────────────┘
```

**Преимущества подхода:**
- **Модульность** - игровую логику можно заменить без пересборки GUI
- **Совместимость** - одинаковый GUI для разных реализаций игр
- **Тестирование** - можно загружать mock-библиотеки
- **Расширяемость** - поддержка новых игр через тот же интерфейс

## Структура интерфейса

### TetrisLibrary_t - Контракт с библиотекой
**Файл:** `library.h:14-27`

```c
typedef struct {
    // ОБЯЗАТЕЛЬНЫЕ функции API (по спецификации)
    void (*userInput)(UserAction_t action, bool hold);
    GameInfo_t (*updateCurrentState)(void);

    // ДОПОЛНИТЕЛЬНЫЕ функции (могут отсутствовать)
    bool (*tetris_init)(void);
    void (*tetris_destroy)(void);
    bool (*tetris_is_game_over)(void);
    void (*tetris_restart_game)(void);

    // Внутренний handle библиотеки
    void* lib_handle;
} TetrisLibrary_t;
```

### Классификация функций

#### Обязательные (API спецификации)
```c
void (*userInput)(UserAction_t action, bool hold);
GameInfo_t (*updateCurrentState)(void);
```
**Требования:** Должны присутствовать в библиотеке, иначе загрузка неуспешна.

#### Дополнительные (расширенный API)
```c
bool (*tetris_init)(void);
void (*tetris_destroy)(void);
bool (*tetris_is_game_over)(void);
void (*tetris_restart_game)(void);
```
**Требования:** Опциональные, GUI адаптируется к их отсутствию.

## Загрузка библиотеки

### library_load() - Основная функция загрузки
**Файл:** `library.c:27-69`

```c
TetrisLibrary_t* library_load(const char* lib_path) {
    if (!lib_path) {
        fprintf(stderr, "Ошибка: не указан путь к библиотеке\n");
        return NULL;
    }

    // Выделяем память для структуры интерфейса
    TetrisLibrary_t* library = calloc(1, sizeof(TetrisLibrary_t));
    if (!library) {
        fprintf(stderr, "Ошибка выделения памяти для интерфейса библиотеки\n");
        return NULL;
    }

    // Загружаем библиотеку
    library->lib_handle = dlopen(lib_path, RTLD_LAZY);
    if (!library->lib_handle) {
        fprintf(stderr, "Ошибка загрузки библиотеки '%s': %s\n", lib_path, dlerror());
        free(library);
        return NULL;
    }

    // Получаем адреса ОБЯЗАТЕЛЬНЫХ функций API
    library->userInput = get_function_address(library->lib_handle, "userInput", true);
    library->updateCurrentState = get_function_address(library->lib_handle, "updateCurrentState", true);

    // Проверяем, что обязательные функции загружены
    if (!library->userInput || !library->updateCurrentState) {
        fprintf(stderr, "Ошибка: не удалось загрузить обязательные функции API\n");
        library_unload(library);
        return NULL;
    }

    // Получаем адреса ДОПОЛНИТЕЛЬНЫХ функций (могут отсутствовать)
    library->tetris_init = get_function_address(library->lib_handle, "tetris_init", false);
    library->tetris_destroy = get_function_address(library->lib_handle, "tetris_destroy", false);
    library->tetris_is_game_over = get_function_address(library->lib_handle, "tetris_is_game_over", false);
    library->tetris_restart_game = get_function_address(library->lib_handle, "tetris_restart_game", false);

    return library;
}
```

### Алгоритм загрузки

#### 1. Валидация параметров
```c
if (!lib_path) {
    fprintf(stderr, "Ошибка: не указан путь к библиотеке\n");
    return NULL;
}
```

#### 2. Выделение памяти интерфейса
```c
TetrisLibrary_t* library = calloc(1, sizeof(TetrisLibrary_t));
```
**`calloc`** обнуляет все поля структуры, что важно для проверок NULL.

#### 3. Загрузка динамической библиотеки
```c
library->lib_handle = dlopen(lib_path, RTLD_LAZY);
```

**Флаги dlopen:**
- **`RTLD_LAZY`** - символы загружаются при первом вызове (ленивая загрузка)
- **`RTLD_NOW`** - все символы загружаются немедленно (не используется)

#### 4. Загрузка обязательных символов
```c
library->userInput = get_function_address(library->lib_handle, "userInput", true);
library->updateCurrentState = get_function_address(library->lib_handle, "updateCurrentState", true);
```

#### 5. Проверка критических функций
```c
if (!library->userInput || !library->updateCurrentState) {
    library_unload(library);  // Откат при ошибке
    return NULL;
}
```

#### 6. Загрузка дополнительных символов
```c
library->tetris_init = get_function_address(library->lib_handle, "tetris_init", false);
// ... остальные необязательные функции
```

## Получение адресов функций

### get_function_address() - Универсальный загрузчик символов
**Файл:** `library.c:15-21`

```c
static void* get_function_address(void* lib_handle, const char* function_name, bool required) {
    void* func_ptr = dlsym(lib_handle, function_name);
    if (!func_ptr && required) {
        fprintf(stderr, "Ошибка получения адреса функции '%s': %s\n", function_name, dlerror());
    }
    return func_ptr;
}
```

### Обработка ошибок dlsym

#### Успешная загрузка
```c
void* func_ptr = dlsym(lib_handle, "userInput");
// func_ptr != NULL - функция найдена
```

#### Функция не найдена
```c
void* func_ptr = dlsym(lib_handle, "missing_function");
// func_ptr == NULL, dlerror() содержит описание ошибки
```

#### Логика обработки
```c
if (!func_ptr && required) {
    fprintf(stderr, "Ошибка получения адреса функции '%s': %s\n",
            function_name, dlerror());
    // Но возвращаем NULL в любом случае - вызывающий код решает что делать
}
return func_ptr;  // NULL или валидный указатель
```

## Управление жизненным циклом

### library_unload() - Выгрузка библиотеки
**Файл:** `library.c:71-84`

```c
void library_unload(TetrisLibrary_t* library) {
    if (!library) {
        return;
    }

    // Выгружаем библиотеку
    if (library->lib_handle) {
        dlclose(library->lib_handle);
    }

    // Очищаем структуру
    memset(library, 0, sizeof(TetrisLibrary_t));
    free(library);
}
```

**Последовательность очистки:**
1. Проверка валидности указателя
2. Выгрузка dlopen'утой библиотеки
3. Обнуление структуры (безопасность)
4. Освобождение памяти

### library_init_game() - Инициализация игры
**Файл:** `library.c:86-99`

```c
bool library_init_game(TetrisLibrary_t* library) {
    if (!library_is_ready(library)) {
        fprintf(stderr, "Ошибка: библиотека не готова к работе\n");
        return false;
    }

    // Инициализация игры (если функция доступна)
    if (library->tetris_init) {
        return library->tetris_init();
    }

    // Если функции инициализации нет, считаем что всё в порядке
    return true;
}
```

**Graceful degradation:**
- Если `tetris_init` отсутствует → возвращаем `true`
- Предполагается, что библиотека самодостаточна

### library_destroy_game() - Деинициализация игры
**Файл:** `library.c:101-110`

```c
void library_destroy_game(TetrisLibrary_t* library) {
    if (!library_is_ready(library)) {
        return;
    }

    // Деинициализация игры (если функция доступна)
    if (library->tetris_destroy) {
        library->tetris_destroy();
    }
}
```

### library_is_ready() - Проверка готовности
**Файл:** `library.c:112-117`

```c
bool library_is_ready(TetrisLibrary_t* library) {
    return library &&
           library->lib_handle &&
           library->userInput &&
           library->updateCurrentState;
}
```

**Критерии готовности:**
- Структура выделена и валидна
- Библиотека загружена (`lib_handle != NULL`)
- Обязательные функции API доступны

## Использование в main.c

### Типичная последовательность
```c
// Глобальные переменные
static TetrisLibrary_t* tetris_lib = NULL;

// Инициализация
bool initialize_system(void) {
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

    return true;
}

// Игровой цикл
while (running) {
    // Отправка пользовательского ввода
    tetris_lib->userInput(action, false);

    // Получение состояния игры
    GameInfo_t game_info = tetris_lib->updateCurrentState();

    // Проверка окончания игры (если функция доступна)
    if (tetris_lib->tetris_is_game_over) {
        bool is_game_over = tetris_lib->tetris_is_game_over();
    }

    // Рестарт (если функция доступна)
    if (action == Start && tetris_lib->tetris_restart_game) {
        tetris_lib->tetris_restart_game();
    }
}

// Очистка
void cleanup_system(void) {
    if (tetris_lib) {
        library_destroy_game(tetris_lib);
        library_unload(tetris_lib);
    }
}
```

## Обработка ошибок и надежность

### Проверки валидности
```c
// Перед каждым вызовом функции библиотеки
if (library_is_ready(tetris_lib)) {
    tetris_lib->userInput(action, false);
}

// Проверка дополнительных функций
if (tetris_lib && tetris_lib->tetris_is_game_over) {
    bool game_over = tetris_lib->tetris_is_game_over();
}
```

### Восстановление после ошибок
```c
// При ошибке загрузки библиотеки
tetris_lib = library_load(DEFAULT_LIB_PATH);
if (!tetris_lib) {
    // Попытка загрузить fallback библиотеку
    tetris_lib = library_load(FALLBACK_LIB_PATH);
}
```

### Защита от segfault
- Все указатели на функции проверяются перед вызовом
- `library_is_ready()` гарантирует минимальную функциональность
- `memset()` при выгрузке предотвращает use-after-free

## Совместимость и портируемость

### Платформозависимые части

#### Unix/Linux/macOS
```c
#include <dlfcn.h>
void* lib_handle = dlopen("./libtetris.so", RTLD_LAZY);
void* func_ptr = dlsym(lib_handle, "userInput");
dlclose(lib_handle);
```

#### Windows (гипотетическая поддержка)
```c
#include <windows.h>
HMODULE lib_handle = LoadLibrary("tetris.dll");
FARPROC func_ptr = GetProcAddress(lib_handle, "userInput");
FreeLibrary(lib_handle);
```

### Конфигурация путей

#### main.c настройки
```c
#define DEFAULT_LIB_PATH "./libtetris.so"     // Unix/Linux
// #define DEFAULT_LIB_PATH "./libtetris.dylib"  // macOS
// #define DEFAULT_LIB_PATH "./tetris.dll"       // Windows
```

#### Автоматическое определение
```c
#ifdef __APPLE__
    #define LIB_EXTENSION ".dylib"
#elif defined(_WIN32)
    #define LIB_EXTENSION ".dll"
#else
    #define LIB_EXTENSION ".so"
#endif

#define DEFAULT_LIB_PATH "./libtetris" LIB_EXTENSION
```

## Производительность

### Накладные расходы

#### Загрузка библиотеки
- **dlopen()** - однократно при старте программы
- **dlsym()** - для каждой функции при загрузке
- **Общие накладные расходы:** ~1-5ms при инициализации

#### Вызов функций
```c
// Прямой вызов
userInput(action, false);

// Через указатель на функцию
tetris_lib->userInput(action, false);
```
**Разница:** Один дополнительный indirect call, ~1-2 CPU цикла

### Оптимизации

#### Кэширование часто используемых функций
```c
// В критических местах
static void (*cached_userInput)(UserAction_t, bool) = NULL;

if (!cached_userInput && tetris_lib) {
    cached_userInput = tetris_lib->userInput;
}

if (cached_userInput) {
    cached_userInput(action, false);
}
```

#### Batch вызовы
Группировка нескольких операций в одну функцию библиотеки.

## Расширения и будущее развитие

### Поддержка нескольких игр
```c
typedef struct {
    char game_name[32];
    void* (*load_game_api)(void);
    void (*unload_game_api)(void*);
} GameLoader_t;

// Универсальный загрузчик для всех BrickGame игр
TetrisLibrary_t* library_load_game(const char* game_name);
```

### Система плагинов
```c
typedef struct {
    int api_version;
    const char* game_name;
    bool (*init)(void);
    GameInfo_t (*update)(void);
    void (*input)(UserAction_t, bool);
    void (*destroy)(void);
} GamePlugin_t;
```

### Динамическая конфигурация
```c
// Конфигурационный файл с путями к библиотекам
bool library_load_from_config(const char* config_file);
```

### Версионирование API
```c
typedef struct {
    int major_version;
    int minor_version;
    bool backward_compatible;
} ApiVersion_t;

bool library_check_compatibility(TetrisLibrary_t* library);
```