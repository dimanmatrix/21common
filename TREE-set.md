# s21::set - Множество уникальных элементов

## 📚 Содержание

1. [Концепция и назначение](#концепция-и-назначение)
2. [Архитектура и отличия от map](#архитектура-и-отличия-от-map)
3. [Интерфейс и основные операции](#интерфейс-и-основные-операции)
4. [Детальный разбор функций](#детальный-разбор-функций)
5. [Практические примеры](#практические-примеры)
6. [Сравнение с другими контейнерами](#сравнение-с-другими-контейнерами)
7. [Заключение](#заключение)

---

## 🎯 Концепция и назначение

**s21::set** — это **ассоциативный контейнер**, который хранит коллекцию **уникальных элементов** в **отсортированном порядке**. В отличие от map, set не хранит пары "ключ-значение" — каждый элемент сам является и ключом, и значением.

### Математическая концепция множества:
```
A = {1, 3, 5, 7, 9}  ← Элементы уникальны и упорядочены
B = {2, 4, 6, 8}
A ∪ B = {1, 2, 3, 4, 5, 6, 7, 8, 9}  ← Объединение без дубликатов
```

### Ключевые свойства:
✅ **Уникальные элементы** — каждый элемент встречается только один раз  
✅ **Автоматическая сортировка** — элементы упорядочены согласно компаратору  
✅ **Эффективные операции** — O(log n) для поиска, вставки, удаления  
✅ **Итераторы** — обход в отсортированном порядке  
✅ **Операции над множествами** — объединение, пересечение (через алгоритмы)  

### Когда использовать set:
🔍 **Проверка принадлежности** — быстрое определение "есть ли элемент"  
📊 **Уникальные коллекции** — удаление дубликатов из данных  
🔀 **Операции над множествами** — объединение, пересечение, разность  
📈 **Сортированные данные** — поддержание порядка без явной сортировки  
🎯 **Фильтрация** — хранение списка разрешенных/запрещенных значений  

### Типичные применения:
```cpp
s21::set<int> unique_ids;              // Уникальные идентификаторы
s21::set<std::string> keywords;        // Ключевые слова языка программирования
s21::set<char> vowels{'a','e','i','o','u'}; // Гласные буквы  
s21::set<std::string> blacklist;       // Черный список пользователей
s21::set<int> prime_numbers;           // Простые числа
```

---

## 🏗️ Архитектура и отличия от map

### Основные различия с map:

| Характеристика | s21::map | s21::set |
|----------------|----------|----------|
| **Тип элементов** | `std::pair<const Key, T>` | `Key` |
| **Назначение** | Ключ → значение | Уникальные элементы |
| **Доступ по ключу** | `operator[]`, `at()` | Только поиск |
| **KeyOfValue** | `return value.first` | `return value` |
| **Случаи использования** | Словари, индексы | Множества, фильтры |

### Архитектурная диаграмма:

```
┌─────────────────────────────────────┐
│            s21::set                 │  ← Публичный интерфейс
├─────────────────────────────────────┤  
│        KeyOfValue функтор           │  ← Элемент = ключ
│        return value;                │
├─────────────────────────────────────┤
│    RedBlackTree<Key, Key>           │  ← key_type == value_type
├─────────────────────────────────────┤
│      Узлы дерева с элементами       │  ← Хранение данных
└─────────────────────────────────────┘
```

### Внутренняя структура узла:

```cpp
struct Node {
    Key data;              // Элемент (он же ключ) 
    Node* parent;          // Родительский узел
    Node* left, *right;    // Дочерние узлы  
    Color color;           // Красный или черный
};
```

### KeyOfValue функтор для set:

```cpp
struct KeyOfValue {
    const Key& operator()(const value_type& value) const {
        return value;  // Элемент сам является ключом!
    }
};
```

Этот простой функтор показывает ключевое отличие set от map — элемент одновременно является и ключом для поиска, и хранимым значением.

---

## 💻 Технические детали C++ реализации

### Template метапрограммирование и тип-безопасность

```cpp
template <typename Key, typename Compare = std::less<Key>>
class set {
public:
    using key_type = Key;
    using value_type = Key;          // Ключевое отличие: value_type = Key!
    using key_compare = Compare;
    using value_compare = Compare;   // Для set: key_compare == value_compare
    
    // Identity функтор - элемент сам является ключом
    struct KeyOfValue {
        const Key& operator()(const Key& value) const {
            return value;  // Тривиальное извлечение ключа
        }
    };
    
    using tree_type = RedBlackTree<Key, Key, KeyOfValue, Compare>;
};
```

### Memory Layout оптимизации

```cpp
// Сравнение размеров узлов:
sizeof(map_node<int, std::string>) ≈ 64 байта  // pair<int, string> + указатели
sizeof(set_node<int>) ≈ 32 байта               // int + указатели + color

// set экономит память при хранении простых типов
s21::set<int> integers;           // 32 байта на узел
s21::map<int, int> int_to_int;    // ~40 байт на узел (из-за pair)
```

### Const-correctness и immutability

```cpp
// Все итераторы set являются const!
using iterator = typename tree_type::const_iterator;
using const_iterator = typename tree_type::const_iterator;

// Это предотвращает изменение элементов через итераторы:
s21::set<int> s = {1, 2, 3};
auto it = s.begin();
// *it = 42;  // ОШИБКА КОМПИЛЯЦИИ! Элементы неизменяемы

// Причина: изменение элемента может нарушить порядок сортировки дерева
```

### Template специализация для performance

```cpp
// Компилятор может оптимизировать KeyOfValue для простых типов
template<typename T>
struct KeyOfValue {
    // Для POD типов - прямое возвращение
    const T& operator()(const T& value) const noexcept {
        return value;  // Нет накладных расходов
    }
};

// Empty Base Optimization для stateless функторов
static_assert(sizeof(std::less<int>) == 1);  // Пустой компаратор
// В реальной реализации может занимать 0 байт благодаря EBO
```

### Perfect Forwarding в модификаторах

```cpp
// emplace использует perfect forwarding для конструирования на месте
template<typename... Args>
std::pair<iterator, bool> emplace(Args&&... args) {
    // Конструируем элемент прямо в узле дерева
    return tree_.emplace(std::forward<Args>(args)...);
}

// Пример оптимального использования:
struct ComplexType {
    std::string data;
    int id;
    ComplexType(std::string s, int i) : data(std::move(s)), id(i) {}
};

s21::set<ComplexType> complex_set;

// Плохо - создает временный объект
complex_set.insert(ComplexType("hello", 42));

// Хорошо - конструирует объект прямо в узле
complex_set.emplace("hello", 42);  // Нет лишних конструкторов!
```

### SFINAE и концепты (C++20 готовность)

```cpp
// Проверка, что тип поддерживает сравнение
template<typename T>
class set {
    static_assert(std::is_copy_constructible_v<T>, 
                  "set requires CopyConstructible type");
    static_assert(std::is_destructible_v<T>, 
                  "set requires Destructible type");
    
    // С C++20 концептами:
    // requires std::totally_ordered<T> || requires(T a, T b, Compare comp) { 
    //     comp(a, b); 
    // }
};
```

### Exception Safety в операциях множеств

```cpp
// Strong exception safety в merge
void merge(set& other) {
    // Создаем временный контейнер для rollback
    std::vector<typename tree_type::iterator> inserted;
    inserted.reserve(other.size());
    
    try {
        for (auto it = other.begin(); it != other.end();) {
            auto result = tree_.insert(*it);
            if (result.second) {
                inserted.push_back(result.first);
                auto to_erase = it++;
                other.tree_.erase(to_erase);
            } else {
                ++it;
            }
        }
    } catch (...) {
        // Откатываем все изменения при исключении
        for (auto& it : inserted) {
            other.tree_.insert(*it);
            tree_.erase(it);
        }
        throw;
    }
}

// Nothrow гарантия для swap
void swap(set& other) noexcept {
    static_assert(std::is_nothrow_swappable_v<tree_type>);
    tree_.swap(other.tree_);
}
```

### Оптимизация hint-based вставки

```cpp
// Оптимизированная вставка с подсказкой позиции
iterator insert(const_iterator hint, const value_type& value) {
    // Если hint корректен, вставка может быть O(1) вместо O(log n)
    if (hint != end()) {
        auto next = std::next(hint);
        
        // Проверяем, подходит ли позиция
        if ((hint == begin() || key_comp()(*std::prev(hint), value)) &&
            (next == end() || key_comp()(value, *next))) {
            // Hint корректен - быстрая вставка
            return tree_.insert_with_hint(hint, value);
        }
    }
    
    // Fallback на обычную вставку
    return insert(value).first;
}
```

### Типы данных:

```cpp
using key_type = Key;           // Тип ключа
using value_type = Key;         // ⚠️ value_type == key_type !
using reference = value_type&;  
using const_reference = const value_type&;
using size_type = size_t;
using key_compare = Compare;
```

---

## 🔧 Интерфейс и основные операции

### Конструкторы:

```cpp
set();                                          // Пустое множество  
explicit set(const Compare& comp);              // С компаратором
set(std::initializer_list<value_type> items);   // Из списка элементов
set(const set& other);                          // Копирование  
set(set&& other) noexcept;                      // Перемещение
```

### Категории операций:

| Категория | Операции | Сложность | Особенности |
|-----------|----------|-----------|-------------|
| **Итераторы** | `begin()`, `end()` | O(1) | Обход в отсортированном порядке |
| **Размер** | `empty()`, `size()`, `max_size()` | O(1) | Стандартные операции |
| **Модификация** | `insert()`, `erase()`, `clear()` | O(log n) | Сохранение уникальности |
| **Поиск** | `find()`, `contains()` | O(log n) | Основное применение set |
| **Операции над множествами** | `merge()` | O(N log(size+N)) | Объединение множеств |

### Отсутствующие операции (по сравнению с map):

❌ **Нет `operator[]`** — элемент нельзя "создать по ключу"  
❌ **Нет `at()`** — нет отдельного значения для доступа  
❌ **Нет `insert_or_assign()`** — нет значения для обновления  

---

## 🔍 Детальный разбор функций

### 🏗️ Конструкторы

#### Конструктор по умолчанию
```cpp
set() : tree_() {}
```
**Назначение**: Создает пустое множество с компаратором по умолчанию (`std::less<Key>`).  
**Сложность**: O(1)  
**Результат**: Множество готово для добавления элементов.

#### Конструктор из списка инициализации
```cpp  
set(std::initializer_list<value_type> const &items) : tree_() {
    for (const auto& item : items) {
        tree_.insert(item);
    }
}
```
**Назначение**: Создает множество из списка элементов, автоматически удаляя дубликаты.  
**Пример**:
```cpp
s21::set<int> numbers = {5, 2, 8, 2, 1, 8, 3}; 
// Результат: {1, 2, 3, 5, 8} - дубликаты удалены, порядок отсортирован
```

#### Конструктор с кастомным компаратором
```cpp
explicit set(const Compare& comp) : tree_(comp) {}
```
**Применение**:
```cpp
// Множество строк по длине (а не по алфавиту)
struct ByLength {
    bool operator()(const std::string& a, const std::string& b) const {
        return a.length() < b.length();
    }
};

s21::set<std::string, ByLength> words_by_length;
words_by_length.insert("a");     // длина 1
words_by_length.insert("hello"); // длина 5  
words_by_length.insert("hi");    // длина 2
// Порядок обхода: "a", "hi", "hello"
```

### ➕ Модификация множества

#### insert() - добавление элемента
```cpp
std::pair<iterator, bool> insert(const value_type& value) {
    return tree_.insert(value);
}
```
**Поведение**: Вставляет элемент только если его еще нет в множестве.  
**Возвращает**: Пару из итератора на элемент и флага успешности вставки.

```cpp
s21::set<int> numbers;

auto [iter1, inserted1] = numbers.insert(42);  
// inserted1 == true, iter1 указывает на 42

auto [iter2, inserted2] = numbers.insert(42);  // Повторная вставка
// inserted2 == false, iter2 указывает на существующий 42

std::cout << "Size: " << numbers.size();  // Размер: 1 (не 2!)
```

#### erase() - удаление элементов
```cpp
void erase(iterator pos) {
    tree_.erase(pos);
}
```
**Варианты удаления**:
```cpp
s21::set<std::string> words = {"apple", "banana", "cherry"};

// Удаление по итератору
auto it = words.find("banana");
if (it != words.end()) {
    words.erase(it);  // Удаляем "banana"  
}

// Удаление по значению (если реализовано erase(const Key&))
// size_t count = words.erase("apple");  // Вернет 1 если удален, 0 если не найден
```

#### merge() - слияние множеств
```cpp
void merge(set& other) {
    for (auto it = other.begin(); it != other.end();) {
        auto result = tree_.insert(*it);
        if (result.second) {
            auto to_erase = it++;
            other.tree_.erase(to_erase);  // Перемещаем уникальный элемент
        } else {
            ++it;  // Элемент уже есть, пропускаем
        }
    }
}
```
**Назначение**: Перемещает все уникальные элементы из другого множества.  
**Математическая аналогия**: A = A ∪ B, B = B \ A

```cpp
s21::set<int> set1 = {1, 2, 3};
s21::set<int> set2 = {3, 4, 5};

set1.merge(set2);
// set1: {1, 2, 3, 4, 5}  - получил уникальные элементы
// set2: {3}              - остался только дубликат
```

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
**Основное применение set** — быстрый поиск принадлежности элемента множеству.

```cpp
s21::set<std::string> allowed_users = {"alice", "bob", "charlie"};

if (auto it = allowed_users.find(username); it != allowed_users.end()) {
    std::cout << "Access granted for " << *it << '\n';
} else {
    std::cout << "Access denied\n";
}
```

#### contains() - проверка наличия элемента
```cpp
bool contains(const Key& key) const {
    return tree_.contains(key);
}
```
**Удобная альтернатива find()** для простой проверки:

```cpp  
s21::set<char> vowels = {'a', 'e', 'i', 'o', 'u'};

char ch = 'e';
if (vowels.contains(ch)) {
    std::cout << ch << " is a vowel\n";
}

// Вместо более громоздкого:
// if (vowels.find(ch) != vowels.end()) { ... }
```

### ➕ Бонусные функции

#### insert_many() - множественная вставка
```cpp
template <typename... Args>
s21::vector<std::pair<iterator, bool>> insert_many(Args&&... args) {
    return tree_.insert_many(std::forward<Args>(args)...);
}
```
**Эффективная вставка** нескольких элементов:

```cpp
s21::set<int> numbers;
auto results = numbers.insert_many(1, 2, 2, 3, 1, 4);

// Проверяем результаты
for (size_t i = 0; i < results.size(); ++i) {
    auto [iter, inserted] = results[i];
    if (inserted) {
        std::cout << "Inserted: " << *iter << '\n';
    } else {
        std::cout << "Duplicate: " << *iter << '\n';
    }
}
// Вывод: Inserted: 1, 2, 3, 4; Duplicate: 2, 1
// Итоговое множество: {1, 2, 3, 4}
```

---

## 📖 Практические примеры

### Пример 1: Фильтрация уникальных элементов

```cpp
#include "s21_containers.h"
#include <iostream>
#include <vector>

// Функция удаления дубликатов из vector
template<typename T>
s21::set<T> remove_duplicates(const std::vector<T>& vec) {
    s21::set<T> unique_elements;
    
    for (const auto& element : vec) {
        unique_elements.insert(element);  // Дубликаты автоматически игнорируются
    }
    
    return unique_elements;
}

// Использование:
std::vector<int> numbers = {1, 5, 2, 1, 3, 5, 2, 7, 3, 1};
auto unique = remove_duplicates(numbers);

// Вывод в отсортированном порядке: 1, 2, 3, 5, 7
for (int num : unique) {
    std::cout << num << " ";
}
```

### Пример 2: Проверка валидности слов

```cpp
class SpellChecker {
private:
    s21::set<std::string> dictionary_;
    
public:
    // Конструктор загружает словарь
    SpellChecker(std::initializer_list<std::string> words) : dictionary_(words) {}
    
    // Добавление слова в словарь
    void add_word(const std::string& word) {
        dictionary_.insert(word);
    }
    
    // Проверка корректности слова  
    bool is_valid(const std::string& word) const {
        return dictionary_.contains(word);
    }
    
    // Проверка предложения
    std::vector<std::string> find_errors(const std::vector<std::string>& words) const {
        std::vector<std::string> errors;
        
        for (const auto& word : words) {
            if (!is_valid(word)) {
                errors.push_back(word);
            }
        }
        
        return errors;
    }
    
    // Статистика словаря
    size_t dictionary_size() const {
        return dictionary_.size();
    }
    
    // Слияние со другим словарем
    void merge_dictionary(SpellChecker& other) {
        dictionary_.merge(other.dictionary_);
    }
};

// Использование:
SpellChecker english_checker = {
    "hello", "world", "programming", "computer", "science"
};

std::vector<std::string> sentence = {"hello", "wrold", "programming"};
auto errors = english_checker.find_errors(sentence);

if (!errors.empty()) {
    std::cout << "Spelling errors found: ";
    for (const auto& error : errors) {
        std::cout << error << " ";  // Вывод: "wrold"
    }
}
```

### Пример 3: Операции над множествами

```cpp
class SetOperations {
public:
    // Пересечение множеств: A ∩ B  
    template<typename T>
    static s21::set<T> intersection(const s21::set<T>& set1, const s21::set<T>& set2) {
        s21::set<T> result;
        
        // Проходим по меньшему множеству для эффективности
        const auto& smaller = (set1.size() <= set2.size()) ? set1 : set2;
        const auto& larger = (set1.size() <= set2.size()) ? set2 : set1;
        
        for (const auto& element : smaller) {
            if (larger.contains(element)) {
                result.insert(element);
            }
        }
        
        return result;
    }
    
    // Разность множеств: A \ B
    template<typename T>
    static s21::set<T> difference(const s21::set<T>& set1, const s21::set<T>& set2) {
        s21::set<T> result;
        
        for (const auto& element : set1) {
            if (!set2.contains(element)) {
                result.insert(element);
            }
        }
        
        return result;
    }
    
    // Симметричная разность: (A \ B) ∪ (B \ A)
    template<typename T>
    static s21::set<T> symmetric_difference(const s21::set<T>& set1, const s21::set<T>& set2) {
        s21::set<T> diff1 = difference(set1, set2);
        s21::set<T> diff2 = difference(set2, set1);
        
        diff1.merge(diff2);  // Объединяем
        return diff1;
    }
    
    // Проверка подмножества: A ⊆ B
    template<typename T>
    static bool is_subset(const s21::set<T>& subset, const s21::set<T>& superset) {
        if (subset.size() > superset.size()) {
            return false;  // Подмножество не может быть больше
        }
        
        for (const auto& element : subset) {
            if (!superset.contains(element)) {
                return false;
            }
        }
        
        return true;
    }
};

// Применение:
s21::set<int> A = {1, 2, 3, 4, 5};
s21::set<int> B = {3, 4, 5, 6, 7};

auto intersection = SetOperations::intersection(A, B);     // {3, 4, 5}
auto difference = SetOperations::difference(A, B);        // {1, 2}
auto sym_diff = SetOperations::symmetric_difference(A, B); // {1, 2, 6, 7}

s21::set<int> C = {1, 2};
bool is_subset = SetOperations::is_subset(C, A);          // true
```

### Пример 4: Система тегов

```cpp
class TagSystem {
private:
    s21::set<std::string> available_tags_;
    
public:
    // Инициализация системы тегов
    TagSystem(std::initializer_list<std::string> initial_tags) 
        : available_tags_(initial_tags) {}
    
    // Регистрация нового тега
    bool register_tag(const std::string& tag) {
        auto result = available_tags_.insert(tag);
        return result.second;  // true если тег новый
    }
    
    // Проверка валидности тега
    bool is_valid_tag(const std::string& tag) const {
        return available_tags_.contains(tag);
    }
    
    // Валидация набора тегов
    bool validate_tags(const std::vector<std::string>& tags) const {
        for (const auto& tag : tags) {
            if (!is_valid_tag(tag)) {
                return false;
            }
        }
        return true;
    }
    
    // Получение отсортированного списка тегов
    std::vector<std::string> get_all_tags() const {
        std::vector<std::string> result;
        for (const auto& tag : available_tags_) {
            result.push_back(tag);  // Уже в отсортированном порядке!
        }
        return result;
    }
    
    // Поиск тегов по префиксу
    std::vector<std::string> find_tags_by_prefix(const std::string& prefix) const {
        std::vector<std::string> result;
        
        for (const auto& tag : available_tags_) {
            if (tag.substr(0, prefix.length()) == prefix) {
                result.push_back(tag);
            }
        }
        
        return result;
    }
};

// Использование:
TagSystem blog_tags = {
    "cpp", "python", "javascript", "algorithms", "data-structures",
    "web", "backend", "frontend", "database", "tutorial"
};

// Валидация тегов поста
std::vector<std::string> post_tags = {"cpp", "algorithms", "tutorial"};
if (blog_tags.validate_tags(post_tags)) {
    std::cout << "All tags are valid!\n";
}

// Автодополнение по префиксу
auto suggestions = blog_tags.find_tags_by_prefix("data");
// Результат: ["data-structures", "database"]
```

---

## 🆚 Сравнение с другими контейнерами

### set vs unordered_set

| Характеристика | s21::set | std::unordered_set |
|----------------|----------|-------------------|
| **Базовая структура** | Красно-черное дерево | Хеш-таблица |
| **Упорядоченность** | ✅ Автоматическая сортировка | ❌ Неупорядочен |
| **Поиск** | O(log n) | O(1) среднее, O(n) худшее |
| **Вставка/Удаление** | O(log n) | O(1) среднее, O(n) худшее |
| **Итерирование** | В отсортированном порядке | В произвольном порядке |
| **Память** | Меньше overhead | Больше overhead |
| **Стабильность итераторов** | ✅ При модификации | ❌ Могут инвалидироваться |

### set vs vector (с сортировкой)

| Характеристика | s21::set | sorted s21::vector |
|----------------|----------|-------------------|
| **Поиск** | O(log n) | O(log n) binary_search |
| **Вставка** | O(log n) | O(n) - сдвиг элементов |
| **Удаление** | O(log n) | O(n) - сдвиг элементов |
| **Память** | Больше (указатели) | Меньше (непрерывный блок) |
| **Cache locality** | Хуже | Лучше |
| **Подходит для** | Частые модификации | Редкие изменения |

### set vs map

| Характеристика | s21::set | s21::map |
|----------------|----------|----------|
| **Тип элемента** | `Key` | `std::pair<const Key, T>` |
| **Назначение** | Принадлежность множеству | Ассоциативный массив |
| **Доступ к значению** | Нет отдельного значения | `operator[]`, `at()` |
| **Случаи использования** | Фильтры, множества | Словари, индексы |

---

## 🎯 Заключение

### Ключевые преимущества s21::set:

✅ **Автоматическая уникальность** — невозможны дубликаты элементов  
✅ **Встроенная сортировка** — элементы всегда упорядочены  
✅ **Эффективный поиск** — O(log n) проверка принадлежности  
✅ **Математические операции** — естественная поддержка операций над множествами  
✅ **Стабильная производительность** — гарантированные временные сложности  

### Идеальные случаи использования:

🔍 **Фильтрация данных** — списки разрешенных/запрещенных значений  
📊 **Удаление дубликатов** — получение уникальных элементов из коллекций  
🎯 **Принадлежность множеству** — быстрые проверки "содержится ли элемент"  
🔀 **Операции над множествами** — объединение, пересечение, разность  
📋 **Теги и метки** — системы классификации и маркировки  
🗃️ **Индексы и справочники** — отсортированные списки значений  

### Ограничения:

❌ **Неизменяемые элементы** — элемент нельзя модифицировать после вставки  
❌ **Только уникальные элементы** — нет поддержки дубликатов (используйте multiset)  
❌ **Overhead памяти** — дополнительная память на указатели и метаданные  
❌ **Медленнее хеш-таблиц** — O(log n) vs O(1) для простых проверок  

### Альтернативы:

- **std::unordered_set** — для максимальной скорости поиска без необходимости порядка
- **s21::multiset** — если нужны дубликаты элементов
- **s21::vector + sort/unique** — для редко изменяемых данных  
- **простые флаги bool** — для небольшого фиксированного множества элементов

---

### 💡 Рекомендации по использованию

1. **Используйте set** когда нужна **уникальность + порядок + быстрый поиск**
2. **Предпочитайте contains()** вместо `find() != end()` для простых проверок
3. **Используйте insert_many()** для эффективной вставки нескольких элементов
4. **Применяйте merge()** для объединения множеств без копирования элементов
5. **Рассмотрите unordered_set** если порядок элементов не важен, а скорость критична

**s21::set — это мощный инструмент для работы с уникальными отсортированными коллекциями, идеально подходящий для задач, где важна математическая корректность операций над множествами! 🚀**

---

> 📝 **Примечание**: Данная документация основана на реализации s21::set в файле `src/source/headers/s21_set.h` проекта s21_containers. Для более глубокого понимания внутреннего устройства рекомендуется изучить [TREE.md](./TREE.md) и [TREE-map.md](./TREE-map.md).