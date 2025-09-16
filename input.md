# INPUT.H/C - Обработка пользовательского ввода

## Обзор модуля

`gui/cli/input.c` реализует систему обработки пользовательского ввода:
- **Неблокирующее чтение клавиш** из терминала
- **Распознавание escape-последовательностей** для стрелок
- **Маппинг клавиш на игровые действия** согласно спецификации
- **Поддержка альтернативных клавиш** (WASD + стрелки)

## Константы клавиш

### Escape-последовательности стрелок
**Файл:** `input.h:14-17`

```c
#define KEY_ARROW_UP    1001
#define KEY_ARROW_DOWN  1002
#define KEY_ARROW_RIGHT 1003
#define KEY_ARROW_LEFT  1004
```

**Обоснование больших значений:**
- Избегают конфликта с ASCII символами (0-127)
- Легко отличимы от обычных клавиш
- Совместимы с `int` возвращаемым типом

### Обычные клавиши
**Файл:** `input.h:20-23`

```c
#define KEY_ESC         27   // ASCII ESC
#define KEY_ENTER       10   // ASCII LF (Line Feed)
#define KEY_CARRIAGE    13   // ASCII CR (Carriage Return)
#define KEY_SPACE       32   // ASCII Space
```

## API функции

### input_get_key() - Неблокирующее чтение клавиш
**Файл:** `input.c:11-33`

```c
int input_get_key(void) {
    int ch = getchar();
    if (ch == EOF) {
        return -1;  // Нет ввода
    }

    // Обработка escape-последовательностей (стрелки)
    if (ch == KEY_ESC) {
        int ch2 = getchar();
        if (ch2 == '[') {
            int ch3 = getchar();
            switch (ch3) {
                case 'A': return KEY_ARROW_UP;
                case 'B': return KEY_ARROW_DOWN;
                case 'C': return KEY_ARROW_RIGHT;
                case 'D': return KEY_ARROW_LEFT;
            }
        }
        return KEY_ESC; // Обычный ESC
    }

    return ch;
}
```

**Алгоритм обработки escape-последовательностей:**

#### 1. Обычные символы
```c
int ch = getchar();
if (ch == EOF) return -1;  // Нет ввода (неблокирующий режим)
if (ch != KEY_ESC) return ch;  // Обычный ASCII символ
```

#### 2. Escape-последовательности стрелок
```
ESC [ A  →  KEY_ARROW_UP     (↑)
ESC [ B  →  KEY_ARROW_DOWN   (↓)
ESC [ C  →  KEY_ARROW_RIGHT  (→)
ESC [ D  →  KEY_ARROW_LEFT   (←)
```

**Последовательность чтения:**
1. `ch = ESC (27)` - начало последовательности
2. `ch2 = '['` - ANSI CSI (Control Sequence Introducer)
3. `ch3 = 'A'/'B'/'C'/'D'` - код направления

#### 3. Обработка неполных последовательностей
```c
if (ch2 == '[') {
    int ch3 = getchar();
    // Обрабатываем ch3...
}
return KEY_ESC;  // Если последовательность неполная → обычный ESC
```

**Проблема:** При неблокирующем режиме `ch2` или `ch3` могут вернуть EOF, но код обрабатывает это как обычный ESC.

### input_key_to_action() - Маппинг клавиш на действия
**Файл:** `input.c:35-82`

```c
UserAction_t input_key_to_action(int key) {
    switch (key) {
        // Старт игры
        case 's': case 'S':
        case KEY_ENTER: case KEY_CARRIAGE:
            return Start;

        // Пауза
        case 'p': case 'P':
        case KEY_SPACE:
            return Pause;

        // Выход
        case 'q': case 'Q':
        case KEY_ESC:
            return Terminate;

        // Движение влево
        case KEY_ARROW_LEFT:
        case 'a': case 'A':
            return Left;

        // Движение вправо
        case KEY_ARROW_RIGHT:
        case 'd': case 'D':
            return Right;

        // Поворот фигуры
        case KEY_ARROW_UP:
        case 'w': case 'W':
        case 'r': case 'R':
        case 'e': case 'E':
            return Up;

        // Ускоренное падение / мгновенный сброс
        case KEY_ARROW_DOWN:
        case 's': case 'S':
            return Down;

        // Действие (альтернативный поворот)
        case 'x': case 'X':
        case ' ':
            return Action;

        default:
            return -1;  // Неизвестная клавиша
    }
}
```

### Схема управления

#### Основные действия
| Действие | Основные клавиши | Альтернативы | Назначение |
|----------|------------------|--------------|------------|
| **Start** | S, s | Enter, CR | Старт игры / рестарт |
| **Pause** | P, p | Space | Пауза / продолжение |
| **Terminate** | Q, q | Esc | Выход из игры |

#### Движение фигур
| Действие | Стрелки | WASD | Дополнительно | Назначение |
|----------|---------|------|---------------|------------|
| **Left** | ← | A, a | - | Движение влево |
| **Right** | → | D, d | - | Движение вправо |
| **Up** | ↑ | W, w | R, r / E, e | Поворот фигуры |
| **Down** | ↓ | S, s | - | Быстрое падение |
| **Action** | - | X, x | Space | Альтернативный поворот |

#### Особенности маппинга

**1. Конфликт клавиши S:**
```c
case 's': case 'S':
    return Start;  // В меню/game over
// ...
case 's': case 'S':
    return Down;   // В игре
```
**Решение:** Один символ mapped в два действия. Логика обработки зависит от контекста в main.c.

**2. Множественные клавиши для поворота:**
- `↑` - основная (интуитивно понятная)
- `W` - WASD схема
- `R/E` - альтернативы для удобства

**3. Регистронезависимость:**
Все буквенные клавиши поддерживают верхний и нижний регистр.

### input_is_quit_key() - Проверка команд выхода
**Файл:** `input.c:84-86`

```c
bool input_is_quit_key(int key) {
    return (key == 'q' || key == 'Q' || key == KEY_ESC);
}
```

**Использование:**
- Быстрая проверка в игровом цикле
- Приоритет над другими действиями
- Позволяет выйти из любого состояния игры

## Интеграция с игровым циклом

### Типичное использование в main.c
```c
// В игровом цикле
int key = input_get_key();
if (key != -1) {
    // Проверяем команды выхода (высший приоритет)
    if (input_is_quit_key(key)) {
        running = false;
        continue;
    }

    // Преобразуем клавишу в игровое действие
    UserAction_t action = input_key_to_action(key);
    if (action != -1) {
        // Отправляем действие в библиотеку Tetris
        tetris_lib->userInput(action, false);

        // Специальная обработка для смены состояния UI
        if (action == Start) {
            // Переход start screen → playing или game over → playing
        }
    }
}
```

### Последовательность обработки
1. **Чтение клавиши** - неблокирующее
2. **Проверка выхода** - максимальный приоритет
3. **Маппинг на действие** - преобразование в UserAction_t
4. **Отправка в библиотеку** - userInput() вызов
5. **UI логика** - обработка смены экранов

## Особенности реализации

### Неблокирующий режим
```c
// Настройка в terminal.c:
raw.c_cc[VMIN] = 0;   // Минимум символов = 0
raw.c_cc[VTIME] = 0;  // Таймаут = 0
fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
```

**Результат:**
- `getchar()` возвращает EOF если нет ввода
- Позволяет игровому циклу продолжаться без ожидания клавиш
- Нет блокировки UI при отсутствии пользовательского ввода

### Проблемы и ограничения

#### 1. Гонка условий в escape-последовательностях
```c
if (ch == KEY_ESC) {
    int ch2 = getchar();  // Может вернуть EOF в неблокирующем режиме
    if (ch2 == '[') {
        int ch3 = getchar();  // Тоже может вернуть EOF
    }
}
```

**Проблема:** Если пользователь нажимает стрелку, но следующие символы последовательности еще не готовы, функция вернет обычный ESC.

**Возможное решение:** Буферизация неполных последовательностей.

#### 2. Потеря escape-последовательностей
При быстром вводе символы могут "потеряться" между вызовами `input_get_key()`.

#### 3. Платформозависимость
Escape-последовательности могут отличаться на разных терминалах и OS.

### Отладка ввода

#### DEBUG версия функции чтения
```c
#ifdef DEBUG
int input_get_key_debug(void) {
    int key = input_get_key();
    if (key != -1) {
        if (key >= 32 && key <= 126) {
            printf("DEBUG: Key '%c' (%d)\n", key, key);
        } else {
            printf("DEBUG: Key code %d\n", key);
        }
    }
    return key;
}
#endif
```

#### Тестирование клавиш
```bash
# Просмотр raw кодов клавиш
hexdump -C

# Тестирование в терминале
cat -v  # Показывает escape-последовательности
```

## Производительность

### Оптимизации
- **Простые switch/case** - O(1) complexity
- **Минимальные вызовы getchar()** - только при необходимости
- **Отсутствие динамических allocations**

### Частота вызовов
```c
// В main.c игровом цикле (20 FPS)
every 50ms: input_get_key()  // 20 вызовов в секунду
```

**Нагрузка:** Минимальная, функция очень легковесная.

## Расширение функциональности

### Потенциальные улучшения

#### 1. Буферизация escape-последовательностей
```c
static char escape_buffer[4];
static int escape_index = 0;
```

#### 2. Поддержка функциональных клавиш
```c
// F1-F12, Page Up/Down, Home/End
#define KEY_F1 2001
#define KEY_PAGE_UP 2002
```

#### 3. Поддержка модификаторов
```c
// Shift, Ctrl, Alt combinations
#define KEY_CTRL_C 3003
#define KEY_SHIFT_ARROW_UP 3004
```

#### 4. Конфигурируемый маппинг
```c
typedef struct {
    int key_code;
    UserAction_t action;
} KeyMapping_t;

static KeyMapping_t custom_mappings[] = {
    // Пользовательские настройки
};
```

### Совместимость с будущими играми
Модуль спроектирован для повторного использования с другими играми BrickGame серии (Snake, Tanks, etc.) через абстрактный `UserAction_t` enum.