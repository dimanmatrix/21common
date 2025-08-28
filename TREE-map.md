# s21::map - Ассоциативный контейнер "ключ-значение"

## 📚 Содержание

1. [Концепция и назначение](#концепция-и-назначение)
2. [Архитектура и внутреннее устройство](#архитектура-и-внутреннее-устройство)
3. [Интерфейс и основные операции](#интерфейс-и-основные-операции)
4. [Детальный разбор функций](#детальный-разбор-функций)
5. [Практические примеры](#практические-примеры)
6. [Сравнение с другими контейнерами](#сравнение-с-другими-контейнерами)
7. [Заключение](#заключение)

---

## 🎯 Концепция и назначение

**s21::map** — это **ассоциативный контейнер**, который хранит элементы в виде пар "ключ-значение" (`std::pair<const Key, T>`). Основные характеристики:

### Ключевые свойства:
✅ **Уникальные ключи** — каждый ключ встречается только один раз  
✅ **Автоматическая сортировка** — элементы упорядочены по ключам  
✅ **Эффективный доступ** — O(log n) для поиска, вставки, удаления  
✅ **Двунаправленные итераторы** — поддержка `++` и `--`  

### Когда использовать map:
🔍 **Поиск по ключу** — быстрый доступ к значениям  
📊 **Словари и индексы** — соответствие одних данных другим  
📈 **Кэширование результатов** — сохранение вычисленных значений  
🗂️ **Конфигурационные файлы** — пары "параметр-значение"  

### Типичные применения:
```cpp
s21::map<std::string, int> word_count;        // Подсчет частоты слов
s21::map<int, std::string> id_to_name;        // Соответствие ID → имя  
s21::map<std::string, double> stock_prices;   // Цены акций по тикеру
s21::map<char, int> char_frequency;           // Анализ текста
```

---

## 🏗️ Архитектура и внутреннее устройство

### Основа: красно-черное дерево

Map построен на базе **красно-черного дерева**, что обеспечивает:
- **Гарантированную производительность**: O(log n) для всех операций
- **Сбалансированность**: высота дерева ≤ 2 log n
- **Эффективность**: меньше вращений при модификации чем в AVL

```cpp
template <typename Key, typename T, typename Compare = std::less<Key>>
class map {
private:
    using tree_type = RedBlackTree<Key, value_type, KeyOfValue, Compare>;
    tree_type tree_;  // Внутреннее дерево
};
```

### Архитектурная диаграмма:

```
┌─────────────────────────────────────┐
│            s21::map                 │  ← Публичный интерфейс
├─────────────────────────────────────┤
│        KeyOfValue функтор           │  ← Извлечение ключа из pair
├─────────────────────────────────────┤
│     RedBlackTree<Key, pair>         │  ← Базовая структура данных
├─────────────────────────────────────┤
│         Узлы дерева с парами        │  ← Хранение данных
└─────────────────────────────────────┘
```

### Хранение данных:

```cpp
// Каждый узел дерева содержит:
struct Node {
    std::pair<const Key, T> data;  // Пара ключ-значение
    Node* parent;                  // Родительский узел  
    Node* left, *right;           // Дочерние узлы
    Color color;                  // Красный или черный
};
```

### KeyOfValue функтор:

```cpp
struct KeyOfValue {
    const Key& operator()(const value_type& value) const {
        return value.first;  // Извлекаем ключ из пары
    }
};
```

Этот функтор позволяет красно-черному дереву работать с парами, извлекая ключ для сравнения.

---

## 🔧 Интерфейс и основные операции

### Типы данных:

```cpp
using key_type = Key;                           // Тип ключа
using mapped_type = T;                          // Тип значения  
using value_type = std::pair<const Key, T>;     // Тип элемента
using reference = value_type&;                  // Ссылка на элемент
using const_reference = const value_type&;      // Константная ссылка
using size_type = size_t;                       // Тип размера
using key_compare = Compare;                    // Тип компаратора
```

### Конструкторы:

```cpp
map();                                          // Пустой map
explicit map(const Compare& comp);              // С компаратором
map(std::initializer_list<value_type> items);   // Из списка
map(const map& other);                          // Копирование
map(map&& other) noexcept;                      // Перемещение
```

### Основные категории операций:

| Категория | Операции | Сложность |
|-----------|----------|-----------|
| **Доступ к элементам** | `at()`, `operator[]` | O(log n) |
| **Итераторы** | `begin()`, `end()` | O(1) |
| **Размер** | `empty()`, `size()`, `max_size()` | O(1) |
| **Модификация** | `insert()`, `erase()`, `clear()` | O(log n) |
| **Поиск** | `find()`, `contains()` | O(log n) |

---

## 💻 Технические детали C++ реализации

### Template архитектура и метапрограммирование

```cpp
template <typename Key, typename T, typename Compare = std::less<Key>>
class map {
    // KeyOfValue функтор для извлечения ключа из std::pair
    struct KeyOfValue {
        const Key& operator()(const value_type& value) const {
            return value.first;  // Извлекаем ключ из пары
        }
    };
    
    // Alias для базового дерева
    using tree_type = RedBlackTree<Key, value_type, KeyOfValue, Compare>;
};
```

**Ключевые архитектурные решения:**

1. **Template параметр Compare**: Позволяет настраивать порядок сортировки
```cpp
// Стандартный порядок (возрастающий)
s21::map<int, std::string> standard_map;

// Убывающий порядок  
s21::map<int, std::string, std::greater<int>> reverse_map;

// Пользовательский компаратор
struct CustomCompare {
    bool operator()(const std::string& a, const std::string& b) const {
        return a.length() < b.length();  // Сортировка по длине строки
    }
};
s21::map<std::string, int, CustomCompare> length_sorted_map;
```

2. **KeyOfValue функтор**: Абстракция для работы с парами
```cpp
// Это позволяет дереву работать с std::pair<const Key, T>,
// но сравнивать только по ключам (Key)
const Key& key = key_of_value_(node->data);  // Извлекаем ключ из пары
```

### Memory layout и производительность

```cpp
// Размер одного узла в памяти (приблизительно):
sizeof(Node<std::pair<int, std::string>>) = 
    sizeof(std::pair<const int, std::string>) +  // Данные: ~32 байта
    3 * sizeof(void*) +                          // Указатели: 24 байта  
    sizeof(Color) +                              // Цвет: 1 байт
    padding;                                     // Выравнивание: ~7 байт
// Итого: ~64 байта на узел
```

**Оптимизации компилятора:**
- **Empty Base Optimization (EBO)** для функторов без состояния
- **Template instantiation** происходит только для используемых методов
- **Inlining** простых операций как `empty()`, `size()`

### Perfect Forwarding и Modern C++

```cpp
// insert_many использует perfect forwarding
template <typename... Args>
s21::vector<std::pair<iterator, bool>> insert_many(Args&&... args) {
    return tree_.insert_many(std::forward<Args>(args)...);
    //                      ^^^^^^^^^^^^^^^^^^^^^^^^^
    //                      Perfect forwarding сохраняет
    //                      value category аргументов
}

// Использование:
map.insert_many(
    std::make_pair(1, "one"),           // lvalue
    std::pair<int, std::string>(2, "two"), // rvalue
    {3, "three"}                        // braced initialization
);
```

### Exception Safety гарантии

```cpp
// Strong exception safety в operator[]
T& operator[](const Key& key) {
    // Если T() бросает исключение, состояние map не изменится
    auto result = tree_.insert(std::make_pair(key, T()));
    return result.first->second;
}

// Basic exception safety в merge()
void merge(map& other) {
    for (auto it = other.begin(); it != other.end();) {
        auto result = tree_.insert(*it);
        if (result.second) {
            // Удаляем из other только после успешной вставки
            auto to_erase = it++;
            other.tree_.erase(to_erase);
        } else {
            ++it;
        }
    }
    // Если вставка бросает исключение, other остается в валидном состоянии
}
```

### Move Semantics и RAII

```cpp
// Move конструктор - O(1) операция
map(map&& other) noexcept : tree_(std::move(other.tree_)) {}

// Move assignment с self-assignment защитой
map& operator=(map&& other) noexcept {
    if (this != &other) {
        tree_ = std::move(other.tree_);  // Перемещаем внутреннее состояние
    }
    return *this;
}

// RAII - деструктор автоматически вызывает ~tree_type()
~map() = default;  // Компилятор генерирует правильный деструктор
```

---

## 🔍 Детальный разбор функций

### 🏗️ Конструкторы

#### Конструктор по умолчанию
```cpp
map() : tree_() {}
```
**Назначение**: Создает пустой map с компаратором по умолчанию.  
**Сложность**: O(1)  
**Использование**: Когда нужен простой словарь с стандартным порядком сортировки.

#### Конструктор с компаратором
```cpp  
explicit map(const Compare& comp) : tree_(comp) {}
```
**Назначение**: Создает пустой map с пользовательским компаратором.  
**Пример**:
```cpp
// Сортировка в убывающем порядке
s21::map<int, std::string, std::greater<int>> reverse_map;
```

#### Конструктор из списка инициализации
```cpp
map(std::initializer_list<value_type> const &items) : tree_() {
    for (const auto& item : items) {
        tree_.insert(item);
    }
}
```
**Назначение**: Создает map из списка пар.  
**Пример**:
```cpp
s21::map<std::string, int> scores = {
    {"Alice", 95}, 
    {"Bob", 87}, 
    {"Carol", 92}
};
```

### 🔐 Доступ к элементам

#### at() - безопасный доступ
```cpp
T& at(const Key& key) {
    auto it = tree_.find(key);
    if (it == tree_.end()) {
        throw std::out_of_range("map::at: key not found");
    }
    return it->second;
}
```
**Назначение**: Возвращает ссылку на значение по ключу с проверкой существования.  
**Исключения**: `std::out_of_range` если ключ не найден.  
**Когда использовать**: Когда важна безопасность и обработка ошибок.

#### operator[] - доступ с автовставкой
```cpp
T& operator[](const Key& key) {
    auto result = tree_.insert(std::make_pair(key, T()));
    return result.first->second;
}
```
**Назначение**: Возвращает ссылку на значение, создавая элемент если его нет.  
**Особенность**: Автоматически вставляет новый элемент с default-значением.  
**Когда использовать**: Для удобного доступа и модификации.

**Сравнение at() vs operator[]:**

```cpp
s21::map<std::string, int> m;

// operator[] - создает элемент если его нет
m["new_key"] = 42;  // OK: создается пара {"new_key", 0}, потом присваивается 42

// at() - выбрасывает исключение если элемента нет  
try {
    int value = m.at("missing_key");  // Исключение!
} catch (const std::out_of_range& e) {
    // Обработка ошибки
}
```

### 🔄 Модификация

#### insert() - вставка элементов
```cpp
std::pair<iterator, bool> insert(const value_type& value) {
    return tree_.insert(value);
}
```
**Возвращает**: Пару из итератора на элемент и флага успешности.  
**Поведение**: Вставляет только если ключ уникален.

**Варианты использования**:
```cpp
s21::map<int, std::string> m;

// Вариант 1: insert(pair)  
auto result1 = m.insert({1, "one"});
if (result1.second) {
    std::cout << "Inserted successfully\n";
}

// Вариант 2: insert(key, value)
auto result2 = m.insert(2, "two");

// Проверка результата
auto [iter, inserted] = m.insert({1, "ONE"}); // C++17
if (!inserted) {
    std::cout << "Key already exists: " << iter->second << '\n';
}
```

#### insert_or_assign() - вставка или обновление
```cpp
std::pair<iterator, bool> insert_or_assign(const Key& key, const T& obj) {
    auto result = tree_.insert(std::make_pair(key, obj));
    if (!result.second) {
        result.first->second = obj;  // Обновляем существующее значение
    }
    return result;
}
```
**Отличие от insert()**: Обновляет значение если ключ уже существует.  
**Возвращает**: `true` при вставке нового элемента, `false` при обновлении.

#### erase() - удаление элементов  
```cpp
void erase(iterator pos) {
    tree_.erase(pos);
}
```
**Варианты удаления**:
```cpp  
s21::map<int, std::string> m = {{1, "one"}, {2, "two"}, {3, "three"}};

// По итератору
auto it = m.find(2);
if (it != m.end()) {
    m.erase(it);
}

// По ключу (если была бы реализована)  
// size_t count = m.erase(1);  // Возвращает количество удаленных (0 или 1)
```

#### merge() - слияние с другим map
```cpp
void merge(map& other) {
    for (auto it = other.begin(); it != other.end();) {
        auto result = tree_.insert(*it);
        if (result.second) {
            auto to_erase = it++;
            other.tree_.erase(to_erase);  // Перемещаем элемент  
        } else {
            ++it;  // Пропускаем дубликат
        }
    }
}
```
**Назначение**: Перемещает уникальные элементы из другого map.  
**Сложность**: O(N log(size() + N)), где N = other.size().

### 🔍 Поиск и проверка

#### find() - поиск элемента
```cpp
iterator find(const Key& key) {
    return tree_.find(key);
}

const_iterator find(const Key& key) const {
    return tree_.find(key);
}
```
**Возвращает**: Итератор на элемент или `end()` если не найден.  
**Использование**:
```cpp
auto it = m.find("key");
if (it != m.end()) {
    std::cout << "Found: " << it->second << '\n';
} else {
    std::cout << "Not found\n";
}
```

#### contains() - проверка наличия элемента  
```cpp
bool contains(const Key& key) const {
    return tree_.contains(key);
}
```
**Назначение**: Удобная проверка без получения итератора.  
**С++20 совместимость**: Аналог `std::map::contains()`.

```cpp
if (m.contains("key")) {
    // Элемент существует
    auto value = m["key"];  // Безопасный доступ
}
```

### ➕ Бонусные функции

#### insert_many() - множественная вставка
```cpp
template <typename... Args>
s21::vector<std::pair<iterator, bool>> insert_many(Args&&... args) {
    return tree_.insert_many(std::forward<Args>(args)...);
}
```
**Назначение**: Эффективная вставка нескольких элементов за один вызов.  
**Использует**: Perfect forwarding и variadic templates.

**Пример использования**:
```cpp
auto results = m.insert_many(
    std::make_pair(1, "one"),
    std::make_pair(2, "two"), 
    std::make_pair(3, "three")
);

for (const auto& [iter, inserted] : results) {
    if (inserted) {
        std::cout << "Inserted: " << iter->first << " -> " << iter->second << '\n';
    }
}
```

---

## 📖 Практические примеры

### Пример 1: Подсчет частоты слов

```cpp
#include "s21_containers.h" 
#include <iostream>
#include <sstream>

void count_words(const std::string& text) {
    s21::map<std::string, int> word_count;
    std::istringstream iss(text);
    std::string word;
    
    // Подсчитываем слова
    while (iss >> word) {
        word_count[word]++;  // operator[] создает элемент если нет
    }
    
    // Выводим результат (автоматически отсортированный!)
    for (const auto& [word, count] : word_count) {
        std::cout << word << ": " << count << '\n';
    }
}

// Вызов:
count_words("hello world hello cpp world hello");

// Вывод:
// cpp: 1
// hello: 3  
// world: 2
```

### Пример 2: Кэш вычислений (мемоизация)

```cpp
class FibonacciCalculator {
private:
    mutable s21::map<int, long long> cache_;
    
public:
    long long fibonacci(int n) const {
        if (n <= 1) return n;
        
        // Проверяем кэш
        auto it = cache_.find(n);
        if (it != cache_.end()) {
            return it->second;  // Возвращаем закэшированный результат
        }
        
        // Вычисляем и кэшируем
        long long result = fibonacci(n-1) + fibonacci(n-2);
        cache_[n] = result;  // Сохраняем в кэш
        
        return result;
    }
    
    void print_cache() const {
        for (const auto& [n, fib] : cache_) {
            std::cout << "fib(" << n << ") = " << fib << '\n';
        }
    }
};

// Использование:
FibonacciCalculator calc;
std::cout << "fib(10) = " << calc.fibonacci(10) << '\n';
calc.print_cache();  // Увидим все промежуточные вычисления
```

### Пример 3: Конфигурационные настройки

```cpp
class ConfigManager {
private:
    s21::map<std::string, std::string> settings_;
    
public:
    // Загрузка из списка инициализации
    ConfigManager() : settings_{
        {"host", "localhost"},
        {"port", "8080"}, 
        {"timeout", "30"},
        {"debug", "false"}
    } {}
    
    // Получение настройки с значением по умолчанию
    std::string get(const std::string& key, const std::string& default_value = "") const {
        auto it = settings_.find(key);
        return (it != settings_.end()) ? it->second : default_value;
    }
    
    // Установка настройки
    void set(const std::string& key, const std::string& value) {
        settings_[key] = value;
    }
    
    // Проверка наличия настройки
    bool has_setting(const std::string& key) const {
        return settings_.contains(key);
    }
    
    // Слияние с другими настройками
    void merge_settings(ConfigManager& other) {
        settings_.merge(other.settings_);
    }
    
    // Вывод всех настроек
    void print_all() const {
        for (const auto& [key, value] : settings_) {
            std::cout << key << " = " << value << '\n';
        }
    }
};
```

### Пример 4: База данных студентов

```cpp
struct Student {
    std::string name;
    int age;
    double gpa;
    
    Student() = default;  // Для operator[]
    Student(const std::string& n, int a, double g) : name(n), age(a), gpa(g) {}
};

class StudentDatabase {
private:
    s21::map<int, Student> students_;  // ID -> Student
    
public:
    // Добавление студента
    bool add_student(int id, const std::string& name, int age, double gpa) {
        auto result = students_.insert(id, Student(name, age, gpa));
        return result.second;  // true если студент добавлен, false если ID занят
    }
    
    // Поиск студента  
    const Student* find_student(int id) const {
        auto it = students_.find(id);
        return (it != students_.end()) ? &it->second : nullptr;
    }
    
    // Обновление GPA
    bool update_gpa(int id, double new_gpa) {
        auto it = students_.find(id);
        if (it != students_.end()) {
            it->second.gpa = new_gpa;
            return true;
        }
        return false;
    }
    
    // Статистика
    void print_statistics() const {
        if (students_.empty()) {
            std::cout << "No students in database\n";
            return;
        }
        
        double sum_gpa = 0.0;
        for (const auto& [id, student] : students_) {
            sum_gpa += student.gpa;
        }
        
        std::cout << "Total students: " << students_.size() << '\n';
        std::cout << "Average GPA: " << sum_gpa / students_.size() << '\n';
    }
    
    // Список всех студентов (отсортированный по ID)
    void print_all_students() const {
        for (const auto& [id, student] : students_) {
            std::cout << "ID: " << id << ", Name: " << student.name 
                     << ", Age: " << student.age << ", GPA: " << student.gpa << '\n';
        }
    }
};
```

---

## 🆚 Сравнение с другими контейнерами

### map vs unordered_map

| Характеристика | s21::map | std::unordered_map |
|----------------|----------|-------------------|
| **Базовая структура** | Красно-черное дерево | Хеш-таблица |
| **Порядок элементов** | Отсортированы по ключу | Неупорядочены |
| **Поиск** | O(log n) | O(1) среднее, O(n) худшее |
| **Вставка** | O(log n) | O(1) среднее, O(n) худшее |  
| **Удаление** | O(log n) | O(1) среднее, O(n) худшее |
| **Обход в порядке ключей** | ✅ Естественно | ❌ Требует сортировки |
| **Память** | Меньше overhead | Больше overhead |
| **Итераторы** | Стабильные при модификации | Могут инвалидироваться |

**Рекомендации по выбору**:
- **map**: Когда нужен порядок, стабильная производительность O(log n)
- **unordered_map**: Когда нужна максимальная скорость поиска O(1), порядок не важен

### map vs vector<pair<Key, T>>

| Характеристика | s21::map | s21::vector<pair> |
|----------------|----------|-------------------|
| **Поиск** | O(log n) | O(n) линейный поиск |
| **Вставка** | O(log n) любое место | O(n) с сохранением порядка |
| **Память** | Больше (указатели) | Меньше (непрерывный блок) |
| **Кэш-дружелюбность** | Хуже | Лучше |
| **Подходит для** | Частый поиск | Редкие изменения, итерирование |

---

## 🎯 Заключение

### Ключевые преимущества s21::map:

✅ **Автоматическая сортировка**: Элементы всегда упорядочены по ключам  
✅ **Гарантированная производительность**: O(log n) для всех операций  
✅ **Уникальность ключей**: Невозможны дубликаты  
✅ **Богатый интерфейс**: Удобные методы доступа и модификации  
✅ **STL-совместимость**: Привычный интерфейс итераторов  

### Когда использовать map:

🔍 **Поиск по ключам** — когда нужен быстрый доступ к значениям  
📊 **Ассоциативные данные** — связь между различными типами данных  
📈 **Кэширование** — сохранение результатов вычислений  
🗂️ **Конфигурации** — хранение параметров приложения  
📋 **Индексы** — создание быстрых справочников  

### Ограничения:

❌ **Неизменяемые ключи** — ключ нельзя изменить после вставки  
❌ **Overhead памяти** — указатели и метаинформация узлов  
❌ **Медленнее хеш-таблиц** — O(log n) vs O(1) для простого поиска  

### Альтернативы:

- **std::unordered_map** — для максимальной скорости поиска
- **s21::multimap** — если нужны дубликаты ключей  
- **s21::vector<pair>** — для редко изменяемых данных
- **Простые массивы** — для небольших фиксированных соответствий

---

### 💡 Финальный совет

s21::map — это **мощный инструмент** для работы с ассоциативными данными. Его использование оправдано когда важны **упорядоченность**, **уникальность ключей** и **стабильная производительность**. Понимание его внутреннего устройства на базе красно-черного дерева поможет эффективно применять этот контейнер в ваших проектах.

**Удачи в освоении STL-контейнеров! 🚀**

---

> 📝 **Примечание**: Данная документация основана на реализации s21::map в файле `src/source/headers/s21_map.h` проекта s21_containers. Для полного понимания рекомендуется также изучить документацию [TREE.md](./TREE.md) по красно-черным деревьям.