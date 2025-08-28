# s21::multiset - Множество с дубликатами

## 📚 Содержание

1. [Концепция и назначение](#концепция-и-назначение)
2. [Ключевые отличия от set](#ключевые-отличия-от-set)
3. [Архитектура и внутреннее устройство](#архитектура-и-внутреннее-устройство)
4. [Интерфейс и основные операции](#интерфейс-и-основные-операции)
5. [Детальный разбор функций](#детальный-разбор-функций)
6. [Работа с дубликатами](#работа-с-дубликатами)
7. [Практические примеры](#практические-примеры)
8. [Сравнение с другими контейнерами](#сравнение-с-другими-контейнерами)
9. [Заключение](#заключение)

---

## 🎯 Концепция и назначение

**s21::multiset** — это **ассоциативный контейнер**, который хранит коллекцию элементов в **отсортированном порядке** с возможностью **дубликатов**. Это ключевое отличие от обычного set, который хранит только уникальные элементы.

### Математическая концепция мультимножества:
```
A = {1, 1, 2, 3, 3, 3, 5}  ← Элементы могут повторяться
|A| = 7                    ← Общий размер (с учетом дубликатов)
|unique(A)| = 4            ← Количество уникальных элементов {1, 2, 3, 5}
count(3) = 3               ← Количество вхождений элемента 3
```

### Ключевые свойства:
✅ **Дубликаты разрешены** — один элемент может встречаться несколько раз  
✅ **Автоматическая сортировка** — все элементы упорядочены согласно компаратору  
✅ **Эффективные операции** — O(log n) для поиска, вставки, удаления  
✅ **Подсчет вхождений** — метод count() возвращает количество дубликатов  
✅ **Диапазонные операции** — equal_range() для работы с группами одинаковых элементов  

### Когда использовать multiset:
📊 **Подсчет частоты** — подсчет количества вхождений элементов  
📈 **Статистический анализ** — работа с повторяющимися данными  
🎵 **Мультимножества** — когда важен порядок и допустимы дубликаты  
📋 **Приоритетные очереди** — с несколькими элементами одного приоритета  
🎯 **Группировка данных** — хранение элементов с их количествами  

### Типичные применения:
```cpp
s21::multiset<int> grades{85, 92, 85, 78, 92, 85}; // Оценки студентов
s21::multiset<std::string> words;                  // Частота слов в тексте  
s21::multiset<double> sensor_data;                 // Показания датчиков
s21::multiset<char> letter_frequency;              // Частота букв
s21::multiset<int> dice_rolls{1,2,2,3,3,3,4,5,6}; // Результаты бросков
```

---

## 🔀 Ключевые отличия от set

### Сравнительная таблица:

| Характеристика | s21::set | s21::multiset |
|----------------|----------|---------------|
| **Дубликаты** | ❌ Запрещены | ✅ Разрешены |
| **insert() возврат** | `std::pair<iterator, bool>` | `iterator` (всегда успешно) |
| **insert() поведение** | Отклоняет дубликаты | Всегда вставляет |
| **count() результат** | 0 или 1 | Любое неотрицательное число |
| **erase(key) результат** | Удаляет максимум 1 элемент | Удаляет все дубликаты |
| **equal_range() размер** | Максимум 1 элемент | Может содержать много элементов |
| **Применение** | Уникальные коллекции | Частотный анализ |

### Демонстрация отличий:

```cpp
// ========== SET ==========
s21::set<int> unique_set;
auto result1 = unique_set.insert(5);  // {5}, result: (iter, true)
auto result2 = unique_set.insert(5);  // {5}, result: (iter, false) - дубликат отклонен
unique_set.size();                    // 1

// ========== MULTISET ==========  
s21::multiset<int> multi_set;
auto iter1 = multi_set.insert(5);     // {5}
auto iter2 = multi_set.insert(5);     // {5, 5} - дубликат принят
multi_set.size();                     // 2
multi_set.count(5);                   // 2 - количество дубликатов
```

---

## 🏗️ Архитектура и внутреннее устройство

### Базовое красно-черное дерево с поддержкой дубликатов

Multiset использует тот же красно-черный **RedBlackTree**, но с ключевой особенностью — вместо `insert()` используется **`insert_multi()`**, который всегда вставляет элемент, даже если такой ключ уже существует.

### Архитектурная диаграмма:

```
┌─────────────────────────────────────┐
│          s21::multiset              │  ← Публичный интерфейс
├─────────────────────────────────────┤
│        KeyOfValue функтор           │  ← Элемент = ключ (как в set)
│        return value;                │
├─────────────────────────────────────┤
│    RedBlackTree<Key, Key> +         │  ← Дерево с поддержкой дубликатов
│    insert_multi() поддержка         │
├─────────────────────────────────────┤
│      Узлы с дубликатами             │  ← Хранение повторяющихся данных
└─────────────────────────────────────┘

---

## 💻 Технические детали C++ реализации

### Архитектурные отличия от set

```cpp
template <typename Key, typename Compare = std::less<Key>>
class multiset {
public:
    using key_type = Key;
    using value_type = Key;
    using key_compare = Compare;
    using value_compare = Compare;
    
    // Тот же KeyOfValue что и у set
    struct KeyOfValue {
        const Key& operator()(const Key& value) const {
            return value;
        }
    };
    
    using tree_type = RedBlackTree<Key, Key, KeyOfValue, Compare>;
    
    // Ключевое отличие: все итераторы const (как у set)
    using iterator = typename tree_type::const_iterator;
    using const_iterator = typename tree_type::const_iterator;
};
```

### Реализация insert с поддержкой дубликатов

```cpp
// Главное отличие от set - используем insert_multi вместо insert_unique
iterator insert(const value_type& value) {
    return tree_.insert_multi(value);  // Всегда вставляет, возвращает iterator
}

// В RedBlackTree::insert_multi():
iterator insert_multi(const value_type& value) {
    Node* parent = nil_;
    Node* current = root_;
    
    // Поиск места для вставки (разрешаем дубликаты!)
    while (current != nil_) {
        parent = current;
        if (comp_(key_of_value_(value), key_of_value_(current->data))) {
            current = current->left;
        } else {
            // Ключевое отличие: не проверяем на равенство!
            // Дубликаты идут в правое поддерево
            current = current->right;
        }
    }
    
    // Создаем новый красный узел
    Node* new_node = new Node(value);
    new_node->color = RED;
    new_node->parent = parent;
    new_node->left = new_node->right = nil_;
    
    // Вставляем в дерево
    if (parent == nil_) {
        root_ = new_node;
    } else if (comp_(key_of_value_(value), key_of_value_(parent->data))) {
        parent->left = new_node;
    } else {
        parent->right = new_node;
    }
    
    insert_fixup(new_node);  // Восстанавливаем свойства RB-дерева
    ++size_;
    
    return iterator(new_node, this);
}
```

### Эффективная реализация count()

```cpp
size_type count(const key_type& key) const {
    auto range = equal_range(key);
    
    // Оптимизация: используем std::distance для подсчета
    return std::distance(range.first, range.second);
    
    // Альтернативная реализация через обход:
    // size_type count = 0;
    // for (auto it = range.first; it != range.second; ++it) {
    //     ++count;
    // }
    // return count;
}
```

### Оптимизированный equal_range

```cpp
std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
    // Находим первый элемент >= key (lower_bound)
    Node* lower = lower_bound_node(key);
    
    // Находим первый элемент > key (upper_bound)  
    Node* upper = upper_bound_node(key);
    
    return std::make_pair(const_iterator(lower, this), 
                         const_iterator(upper, this));
}

// Оптимизированная реализация через одну итерацию
std::pair<const_iterator, const_iterator> equal_range_optimized(const key_type& key) const {
    Node* current = root_;
    Node* lower_result = nil_;
    Node* upper_result = nil_;
    
    while (current != nil_) {
        if (comp_(key_of_value_(current->data), key)) {
            // current->data < key, идем вправо
            current = current->right;
        } else if (comp_(key, key_of_value_(current->data))) {
            // key < current->data, текущий узел может быть upper_bound
            upper_result = current;
            current = current->left;
        } else {
            // key == current->data, найден элемент
            // Ищем начало диапазона в левом поддереве
            Node* left_search = current->left;
            while (left_search != nil_) {
                if (comp_(key_of_value_(left_search->data), key)) {
                    left_search = left_search->right;
                } else {
                    lower_result = left_search;
                    left_search = left_search->left;
                }
            }
            if (lower_result == nil_) lower_result = current;
            
            // Ищем конец диапазона в правом поддереве
            Node* right_search = current->right;  
            while (right_search != nil_) {
                if (comp_(key, key_of_value_(right_search->data))) {
                    upper_result = right_search;
                    right_search = right_search->left;
                } else {
                    right_search = right_search->right;
                }
            }
            
            break;
        }
    }
    
    return std::make_pair(const_iterator(lower_result, this),
                         const_iterator(upper_result, this));
}
```

### Exception Safety в операциях с дубликатами

```cpp
// Strong exception safety при удалении одного элемента
void erase(const_iterator pos) {
    if (pos == end()) return;
    
    Node* node_to_delete = pos.node_;
    
    // Сохраняем информацию для rollback (если нужно)
    Node backup = *node_to_delete;
    
    try {
        tree_.erase_node(node_to_delete);
        --size_;
    } catch (...) {
        // В случае исключения узел уже может быть поврежден
        // Полный rollback сложен, поэтому обеспечиваем basic safety
        throw;
    }
}

// Basic exception safety при удалении всех дубликатов
size_type erase(const key_type& key) {
    auto range = equal_range(key);
    size_type count = 0;
    
    // Собираем все узлы для удаления
    std::vector<Node*> nodes_to_delete;
    for (auto it = range.first; it != range.second; ++it) {
        nodes_to_delete.push_back(it.node_);
        ++count;
    }
    
    // Удаляем по одному (если исключение - часть удалится)
    for (Node* node : nodes_to_delete) {
        try {
            tree_.erase_node(node);
            --size_;
        } catch (...) {
            // Продолжаем удаление остальных узлов
            // Контейнер остается в валидном состоянии
        }
    }
    
    return count;
}
```

### Template optimization для частых операций

```cpp
// Специализация для trivially copyable типов
template<typename T>
typename std::enable_if_t<std::is_trivially_copyable_v<T>, void>
bulk_insert(std::initializer_list<T> values) {
    // Для простых типов можем использовать более эффективные алгоритмы
    tree_.reserve_approximate(values.size());  // Подготовка памяти
    
    for (const auto& value : values) {
        tree_.insert_multi_fast(value);  // Упрощенная вставка
    }
}

// Общая версия с полной проверкой исключений
template<typename T>
typename std::enable_if_t<!std::is_trivially_copyable_v<T>, void>
bulk_insert(std::initializer_list<T> values) {
    for (const auto& value : values) {
        try {
            tree_.insert_multi(value);
        } catch (...) {
            // Для сложных типов обеспечиваем exception safety
            throw;
        }
    }
}
```

### Memory layout с дубликатами

```cpp
// Пример размещения дубликатов в дереве:
//
//         [5B]
//        /    \
//    [3R]      [8B]
//   /  \       /   \
// [1B] [5R]  [7R] [9R]
//      /
//   [5B]
//
// Элемент 5 встречается 3 раза: в корне, в правом потомке 3, и в левом потомке первого 5
// Все дубликаты размещаются согласно правилу: равные элементы идут вправо

// Статистика памяти для multiset<int> с 1000 элементов:
// - Уникальных значений: 100
// - Дубликатов: 900  
// - Средних дубликатов на значение: 9
// - Размер дерева: 1000 узлов × 32 байта = 32KB
// - Высота дерева: ≤ 2 × log₂(1000) ≈ 20 уровней
// - Поиск любого элемента: ≤ 20 сравнений
```
├─────────────────────────────────────┤
│    RedBlackTree::insert_multi()     │  ← Разрешает дубликаты!
├─────────────────────────────────────┤
│   Узлы дерева с повторяющимися      │  ← Хранение данных
│            элементами               │
└─────────────────────────────────────┘
```

### Размещение дубликатов в дереве:

```
Дубликаты размещаются СПРАВА от существующих элементов:

     [3B]
    /    \
  [1R]   [5R]
          /  \
       [3R]  [7B]  ← Дубликат 3 размещается справа
               \
              [3R]  ← Еще один дубликат 3
```

Это гарантирует:
- **Стабильность порядка вставки** среди одинаковых элементов
- **Корректность итераторов** при обходе дубликатов
- **Эффективность equal_range()** — все дубликаты расположены подряд

### Типы данных (идентичны set):

```cpp
using key_type = Key;           // Тип ключа
using value_type = Key;         // value_type == key_type
using reference = value_type&;  
using const_reference = const value_type&;
using size_type = size_t;
using key_compare = Compare;
```

---

## 🔧 Интерфейс и основные операции

### Конструкторы:

```cpp
multiset();                                         // Пустое мультимножество
explicit multiset(const Compare& comp);             // С компаратором  
multiset(std::initializer_list<value_type> items);  // Из списка (с дубликатами)
multiset(const multiset& other);                   // Копирование
multiset(multiset&& other) noexcept;               // Перемещение
```

### Категории операций:

| Категория | Операции | Особенности multiset |
|-----------|----------|---------------------|
| **Итераторы** | `begin()`, `end()` | Обход всех элементов включая дубликаты |
| **Размер** | `empty()`, `size()`, `max_size()` | size() учитывает все дубликаты |
| **Модификация** | `insert()`, `erase()`, `clear()` | insert() всегда успешен |
| **Поиск** | `find()`, `contains()`, **`count()`** | count() может вернуть > 1 |
| **Диапазоны** | **`equal_range()`**, `lower_bound()`, `upper_bound()` | Работа с группами дубликатов |

### Новые/измененные операции по сравнению с set:

✅ **`count(key)`** — возвращает количество элементов с данным ключом  
✅ **`equal_range(key)`** — возвращает диапазон всех дубликатов  
✅ **`insert(value)`** — возвращает только итератор (всегда успешно)  
✅ **`erase(key)`** — удаляет **все** дубликаты с данным ключом  

---

## 🔍 Детальный разбор функций

### 🏗️ Конструкторы

#### Конструктор из списка инициализации
```cpp
multiset(std::initializer_list<value_type> const &items) : tree_() {
    for (const auto& item : items) {
        tree_.insert_multi(item);  // ⚡ Ключевое отличие: insert_multi!
    }
}
```
**Особенность**: Использует `insert_multi()` вместо `insert()`, что позволяет вставить все элементы включая дубликаты.

```cpp
s21::multiset<int> numbers = {3, 1, 4, 1, 5, 9, 2, 6, 5};
// Результат: {1, 1, 2, 3, 4, 5, 5, 6, 9} - все элементы сохранены!
std::cout << "Size: " << numbers.size(); // 9 (не 7 как было бы в set)
```

### ➕ Модификация с поддержкой дубликатов

#### insert() - всегда успешная вставка
```cpp
iterator insert(const value_type& value) {
    return tree_.insert_multi(value);  // Всегда успешно!
}
```
**Ключевое отличие**: Возвращает только **итератор** (не пару), поскольку вставка всегда успешна.

```cpp
s21::multiset<int> ms;

auto iter1 = ms.insert(42);  // Вставляем первый 42
auto iter2 = ms.insert(42);  // Вставляем второй 42 
auto iter3 = ms.insert(42);  // Вставляем третий 42

std::cout << ms.size();      // 3
std::cout << ms.count(42);   // 3

// Все итераторы валидны и указывают на разные узлы!
std::cout << *iter1 << " " << *iter2 << " " << *iter3; // "42 42 42"
```

#### insert() с подсказкой позиции
```cpp
iterator insert(iterator hint, const value_type& value) {
    (void)hint;  // Подсказка игнорируется в базовой реализации
    return insert(value);
}
```
**Назначение**: Оптимизация для случаев, когда известно примерное место вставки. В данной реализации подсказка игнорируется, но интерфейс соответствует STL.

#### erase() - удаление по ключу
```cpp
size_type erase(const key_type& key) { 
    return tree_.erase(key); 
}
```
**Поведение**: Удаляет **все** элементы с данным ключом, возвращает количество удаленных.

```cpp
s21::multiset<int> ms = {1, 2, 2, 2, 3, 4, 4};

size_t removed = ms.erase(2);  // Удаляем все двойки
// removed == 3, ms стало {1, 3, 4, 4}

size_t not_found = ms.erase(9); // not_found == 0
```

### 🔍 Специализированные операции поиска

#### count() - подсчет дубликатов
```cpp
size_type count(const Key& key) const { 
    return tree_.count(key); 
}
```
**Основное применение multiset** — быстрый подсчет количества вхождений элемента.

```cpp
s21::multiset<char> letters = {'a', 'b', 'a', 'c', 'a', 'b'};

std::cout << "a: " << letters.count('a'); // 3
std::cout << "b: " << letters.count('b'); // 2  
std::cout << "c: " << letters.count('c'); // 1
std::cout << "d: " << letters.count('d'); // 0
```

#### equal_range() - диапазон дубликатов  
```cpp
std::pair<iterator, iterator> equal_range(const Key& key) {
    return tree_.equal_range(key);
}
```
**Назначение**: Возвращает диапазон итераторов `[first, last)`, где все элементы равны ключу.

```cpp
s21::multiset<int> ms = {1, 2, 2, 2, 3, 4};

auto [first, last] = ms.equal_range(2);

// Проходим по всем дубликатам числа 2
for (auto it = first; it != last; ++it) {
    std::cout << *it << " ";  // Вывод: "2 2 2"
}

std::cout << "Distance: " << std::distance(first, last); // 3
```

#### lower_bound() и upper_bound()
```cpp
iterator lower_bound(const Key& key) { return tree_.lower_bound(key); }
iterator upper_bound(const Key& key) { return tree_.upper_bound(key); }
```

**Применение для multiset**:
```cpp
s21::multiset<int> ms = {1, 3, 3, 3, 5, 7};

auto lower = ms.lower_bound(3);  // Итератор на первый 3
auto upper = ms.upper_bound(3);  // Итератор на элемент после последнего 3

// Диапазон [lower, upper) содержит все дубликаты 3
for (auto it = lower; it != upper; ++it) {
    std::cout << *it << " ";     // Вывод: "3 3 3"
}
```

### ➕ Бонусные функции

#### insert_many() для multiset
```cpp
template <typename... Args>
s21::vector<std::pair<iterator, bool>> insert_many(Args&&... args) {
    s21::vector<std::pair<iterator, bool>> results;
    results.reserve(sizeof...(args));
    
    auto insert_single = [&](auto&& arg) {
        iterator it = insert(std::forward<decltype(arg)>(arg));
        results.push_back(std::make_pair(it, true)); // Всегда true!
    };
    
    (insert_single(std::forward<Args>(args)), ...);
    return results;
}
```
**Особенность**: Все вставки возвращают `true` (успешно), поскольку multiset принимает все элементы.

```cpp
s21::multiset<int> ms;
auto results = ms.insert_many(1, 1, 2, 2, 2, 3);

// Все 6 вставок успешны, итоговый размер: 6
for (const auto& [iter, success] : results) {
    std::cout << *iter << ":" << success << " "; // "1:1 1:1 2:1 2:1 2:1 3:1"
}
```

---

## 🔢 Работа с дубликатами

### Основные принципы размещения дубликатов

#### 1. Порядок вставки среди равных элементов:
```cpp
s21::multiset<int> ms;
ms.insert(5);  // Первый 5
ms.insert(3);  // Единственный 3  
ms.insert(5);  // Второй 5
ms.insert(5);  // Третий 5

// Порядок в дереве: 3, 5₁, 5₂, 5₃
// Все дубликаты 5 расположены подряд
```

#### 2. Итерирование через дубликаты:
```cpp
s21::multiset<int> ms = {1, 2, 2, 2, 3};

for (auto it = ms.begin(); it != ms.end(); ++it) {
    std::cout << *it << " ";  // Вывод: "1 2 2 2 3"
}
// Все дубликаты обходятся в правильном порядке
```

#### 3. Работа с диапазонами дубликатов:
```cpp
s21::multiset<std::string> words = {"apple", "banana", "apple", "cherry", "apple"};

// Подсчет конкретного слова
auto apple_count = words.count("apple");  // 3

// Получение всех вхождений
auto [first, last] = words.equal_range("apple");

// Изменение всех дубликатов (например, удаление)
words.erase(first, last);  // Удаляем все "apple"
// Результат: {"banana", "cherry"}
```

### Алгоритмы работы с дубликатами

#### Частотный анализ:
```cpp
template<typename T>
void print_frequency(const s21::multiset<T>& ms) {
    auto current = ms.begin();
    
    while (current != ms.end()) {
        T value = *current;
        size_t count = ms.count(value);
        
        std::cout << value << ": " << count << " times\n";
        
        // Переходим к следующему уникальному элементу
        auto [first, last] = ms.equal_range(value);
        current = last;
    }
}

// Применение:
s21::multiset<char> letters = {'a', 'b', 'a', 'c', 'a', 'b'};
print_frequency(letters);
// Вывод:
// a: 3 times  
// b: 2 times
// c: 1 times
```

#### Удаление только первого вхождения:
```cpp
template<typename T>
bool erase_first(s21::multiset<T>& ms, const T& value) {
    auto it = ms.find(value);
    if (it != ms.end()) {
        ms.erase(it);  // Удаляем только один элемент
        return true;
    }
    return false;
}

// Применение:
s21::multiset<int> numbers = {1, 2, 2, 2, 3};
erase_first(numbers, 2);  // numbers стало {1, 2, 2, 3}
```

---

## 📖 Практические примеры

### Пример 1: Анализ частоты слов

```cpp
#include "s21_containers.h" 
#include <iostream>
#include <sstream>
#include <string>

class WordFrequencyAnalyzer {
private:
    s21::multiset<std::string> words_;
    
public:
    // Добавление текста для анализа
    void add_text(const std::string& text) {
        std::istringstream iss(text);
        std::string word;
        
        while (iss >> word) {
            // Приводим к нижнему регистру для унификации
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            words_.insert(word);  // multiset принимает все слова
        }
    }
    
    // Получение частоты конкретного слова
    size_t get_frequency(const std::string& word) const {
        std::string lower_word = word;
        std::transform(lower_word.begin(), lower_word.end(), lower_word.begin(), ::tolower);
        return words_.count(lower_word);
    }
    
    // Топ-N наиболее частых слов
    std::vector<std::pair<std::string, size_t>> get_top_words(size_t n = 10) const {
        std::vector<std::pair<std::string, size_t>> result;
        auto current = words_.begin();
        
        while (current != words_.end() && result.size() < n) {
            std::string word = *current;
            size_t count = words_.count(word);
            
            result.emplace_back(word, count);
            
            // Переходим к следующему уникальному слову
            auto [first, last] = words_.equal_range(word);
            current = last;
        }
        
        // Сортируем по убыванию частоты
        std::sort(result.begin(), result.end(), 
                 [](const auto& a, const auto& b) {
                     return a.second > b.second;
                 });
        
        return result;
    }
    
    // Общая статистика
    void print_statistics() const {
        std::cout << "Total words: " << words_.size() << '\n';
        
        // Подсчитаем уникальные слова
        size_t unique_count = 0;
        auto current = words_.begin();
        
        while (current != words_.end()) {
            unique_count++;
            auto [first, last] = words_.equal_range(*current);
            current = last;
        }
        
        std::cout << "Unique words: " << unique_count << '\n';
    }
};

// Использование:
WordFrequencyAnalyzer analyzer;
analyzer.add_text("the quick brown fox jumps over the lazy dog the fox is quick");

auto top_words = analyzer.get_top_words(5);
for (const auto& [word, count] : top_words) {
    std::cout << word << ": " << count << " times\n";
}
// Вывод:
// the: 3 times
// fox: 2 times  
// quick: 2 times
// brown: 1 times
// jumps: 1 times
```

### Пример 2: Система оценок студентов

```cpp
class GradeManager {
private:
    s21::multiset<int> grades_;
    
public:
    // Добавление оценки
    void add_grade(int grade) {
        if (grade >= 0 && grade <= 100) {
            grades_.insert(grade);
        }
    }
    
    // Добавление нескольких оценок
    void add_grades(std::initializer_list<int> grade_list) {
        for (int grade : grade_list) {
            add_grade(grade);
        }
    }
    
    // Статистика по оценкам
    void print_grade_distribution() const {
        std::cout << "Grade Distribution:\n";
        
        // Группируем по диапазонам
        auto count_range = [this](int min_grade, int max_grade) {
            size_t count = 0;
            for (auto it = grades_.lower_bound(min_grade); 
                 it != grades_.upper_bound(max_grade) && it != grades_.end(); 
                 ++it) {
                count++;
            }
            return count;
        };
        
        std::cout << "A (90-100): " << count_range(90, 100) << '\n';
        std::cout << "B (80-89):  " << count_range(80, 89) << '\n'; 
        std::cout << "C (70-79):  " << count_range(70, 79) << '\n';
        std::cout << "D (60-69):  " << count_range(60, 69) << '\n';
        std::cout << "F (0-59):   " << count_range(0, 59) << '\n';
    }
    
    // Средний балл
    double get_average() const {
        if (grades_.empty()) return 0.0;
        
        int sum = 0;
        for (int grade : grades_) {
            sum += grade;
        }
        
        return static_cast<double>(sum) / grades_.size();
    }
    
    // Медиана
    double get_median() const {
        if (grades_.empty()) return 0.0;
        
        auto it = grades_.begin();
        std::advance(it, grades_.size() / 2);
        
        if (grades_.size() % 2 == 0) {
            // Четное количество элементов - среднее арифметическое двух средних
            auto prev_it = it;
            --prev_it;
            return (*prev_it + *it) / 2.0;
        } else {
            // Нечетное количество - средний элемент
            return *it;
        }
    }
    
    // Мода (наиболее частая оценка)
    std::vector<int> get_mode() const {
        if (grades_.empty()) return {};
        
        std::vector<int> modes;
        size_t max_count = 0;
        auto current = grades_.begin();
        
        while (current != grades_.end()) {
            int grade = *current;
            size_t count = grades_.count(grade);
            
            if (count > max_count) {
                max_count = count;
                modes.clear();
                modes.push_back(grade);
            } else if (count == max_count) {
                modes.push_back(grade);
            }
            
            // Переходим к следующему уникальному значению
            auto [first, last] = grades_.equal_range(grade);
            current = last;
        }
        
        return modes;
    }
    
    // Удаление конкретной оценки (первого вхождения)
    bool remove_grade(int grade) {
        auto it = grades_.find(grade);
        if (it != grades_.end()) {
            grades_.erase(it);
            return true;
        }
        return false;
    }
    
    // Показать все оценки
    void print_all_grades() const {
        std::cout << "All grades: ";
        for (int grade : grades_) {
            std::cout << grade << " ";
        }
        std::cout << '\n';
    }
};

// Использование:
GradeManager manager;
manager.add_grades({85, 92, 78, 85, 96, 82, 85, 88, 92, 90});

manager.print_all_grades();        // Все оценки в порядке возрастания
manager.print_grade_distribution(); // Распределение по буквенным оценкам

std::cout << "Average: " << manager.get_average() << '\n';
std::cout << "Median: " << manager.get_median() << '\n';

auto modes = manager.get_mode();
std::cout << "Mode(s): ";
for (int mode : modes) {
    std::cout << mode << " ";
}
std::cout << '\n';
```

### Пример 3: Приоритетная очередь с дубликатами

```cpp
template<typename T, typename Compare = std::less<T>>
class PriorityMultiQueue {
private:
    // Используем greater для получения max-heap поведения
    s21::multiset<T, std::greater<T>> priorities_;
    
public:
    // Добавление задачи с приоритетом
    void push(const T& priority) {
        priorities_.insert(priority);
    }
    
    // Получение задачи с наивысшим приоритетом
    T top() const {
        if (empty()) {
            throw std::runtime_error("Queue is empty");
        }
        return *priorities_.begin(); // Наивысший приоритет первый (greater)
    }
    
    // Удаление задачи с наивысшим приоритетом
    void pop() {
        if (!empty()) {
            auto it = priorities_.begin();
            priorities_.erase(it);  // Удаляем только одну задачу
        }
    }
    
    // Размер очереди
    size_t size() const {
        return priorities_.size();
    }
    
    // Проверка пустоты
    bool empty() const {
        return priorities_.empty();
    }
    
    // Количество задач с данным приоритетом
    size_t count_priority(const T& priority) const {
        return priorities_.count(priority);
    }
    
    // Показать все приоритеты
    void show_queue() const {
        std::cout << "Priority queue contents: ";
        for (const auto& priority : priorities_) {
            std::cout << priority << " ";
        }
        std::cout << '\n';
    }
};

// Использование:
PriorityMultiQueue<int> task_queue;

// Добавляем задачи с разными приоритетами (больше = выше приоритет)
task_queue.push(5);  // Средний приоритет  
task_queue.push(10); // Высокий приоритет
task_queue.push(1);  // Низкий приоритет
task_queue.push(10); // Еще одна задача с высоким приоритетом
task_queue.push(5);  // Еще одна со средним

task_queue.show_queue(); // "10 10 5 5 1" - отсортировано по убыванию

std::cout << "Tasks with priority 10: " << task_queue.count_priority(10) << '\n'; // 2

// Обработка задач по приоритету
while (!task_queue.empty()) {
    std::cout << "Processing task with priority: " << task_queue.top() << '\n';
    task_queue.pop();
}
// Вывод:
// Processing task with priority: 10
// Processing task with priority: 10  
// Processing task with priority: 5
// Processing task with priority: 5
// Processing task with priority: 1
```

### Пример 4: Анализ результатов соревнований

```cpp
struct Athlete {
    std::string name;
    double time;  // Время забега в секундах
    
    Athlete(const std::string& n, double t) : name(n), time(t) {}
    
    // Оператор сравнения для сортировки по времени
    bool operator<(const Athlete& other) const {
        return time < other.time;  // Меньшее время = лучший результат
    }
    
    bool operator==(const Athlete& other) const {
        return time == other.time; // Одинаковое время
    }
};

class RaceResults {
private:
    s21::multiset<Athlete> results_;
    
public:
    // Добавление результата
    void add_result(const std::string& name, double time) {
        results_.insert(Athlete(name, time));
    }
    
    // Показать все результаты
    void show_results() const {
        std::cout << "Race Results (sorted by time):\n";
        int position = 1;
        
        for (const auto& athlete : results_) {
            std::cout << position++ << ". " << athlete.name 
                     << " - " << athlete.time << "s\n";
        }
    }
    
    // Победители (атлеты с наилучшим временем)
    std::vector<std::string> get_winners() const {
        if (results_.empty()) return {};
        
        std::vector<std::string> winners;
        double best_time = results_.begin()->time;
        
        auto [first, last] = results_.equal_range(Athlete("", best_time));
        
        for (auto it = first; it != last; ++it) {
            winners.push_back(it->name);
        }
        
        return winners;
    }
    
    // Статистика по времени конкретного атлета
    void show_athlete_stats(const std::string& name) const {
        std::vector<double> athlete_times;
        
        for (const auto& result : results_) {
            if (result.name == name) {
                athlete_times.push_back(result.time);
            }
        }
        
        if (athlete_times.empty()) {
            std::cout << name << " has no recorded results.\n";
            return;
        }
        
        double best = *std::min_element(athlete_times.begin(), athlete_times.end());
        double avg = std::accumulate(athlete_times.begin(), athlete_times.end(), 0.0) / athlete_times.size();
        
        std::cout << name << " statistics:\n";
        std::cout << "  Races: " << athlete_times.size() << '\n';
        std::cout << "  Best time: " << best << "s\n";
        std::cout << "  Average time: " << avg << "s\n";
    }
};

// Использование:
RaceResults marathon;

// Добавляем результаты (некоторые атлеты участвуют несколько раз)
marathon.add_result("Alice", 125.5);
marathon.add_result("Bob", 130.2);  
marathon.add_result("Alice", 123.8);  // Улучшила результат
marathon.add_result("Carol", 128.1);
marathon.add_result("Bob", 128.1);    // Такой же результат как у Carol
marathon.add_result("Dave", 123.8);   // Такой же как лучший у Alice

marathon.show_results();
// Вывод:
// 1. Alice - 123.8s
// 2. Dave - 123.8s  
// 3. Alice - 125.5s
// 4. Carol - 128.1s
// 5. Bob - 128.1s
// 6. Bob - 130.2s

auto winners = marathon.get_winners();
std::cout << "Winners (tied for 1st place): ";
for (const auto& winner : winners) {
    std::cout << winner << " ";
}
// Вывод: "Alice Dave"

marathon.show_athlete_stats("Alice");
// Вывод:
// Alice statistics:
//   Races: 2  
//   Best time: 123.8s
//   Average time: 124.65s
```

---

## 🆚 Сравнение с другими контейнерами

### multiset vs set

| Характеристика | s21::set | s21::multiset |
|----------------|----------|---------------|
| **Уникальность** | Только уникальные элементы | Дубликаты разрешены |
| **insert() результат** | `pair<iterator, bool>` | `iterator` |
| **count() результат** | 0 или 1 | 0, 1, 2, 3... |
| **Размер при дубликатах** | Не растет | Растет с каждым дубликатом |
| **Применение** | Принадлежность множеству | Частотный анализ |
| **Производительность** | Одинаковая | Одинаковая |

### multiset vs unordered_multiset

| Характеристика | s21::multiset | std::unordered_multiset |
|----------------|---------------|-------------------------|
| **Базовая структура** | Красно-черное дерево | Хеш-таблица |
| **Упорядоченность** | ✅ Отсортированы | ❌ Неупорядочены |
| **Поиск** | O(log n) | O(1) среднее |
| **Вставка** | O(log n) | O(1) среднее |
| **Итерирование** | В отсортированном порядке | В произвольном порядке |
| **equal_range()** | Смежные элементы | Разбросанные элементы |

### multiset vs map

| Характеристика | s21::multiset | s21::map |
|----------------|---------------|----------|
| **Тип элементов** | `Key` | `pair<const Key, T>` |
| **Дубликаты ключей** | ✅ Разрешены | ❌ Запрещены |
| **Доступ к значению** | Элемент = ключ | `operator[]`, `at()` |
| **Случаи использования** | Частотный анализ | Ассоциативные массивы |

### multiset vs priority_queue

| Характеристика | s21::multiset | std::priority_queue |
|----------------|---------------|-------------------|
| **Доступ к элементам** | Итераторы, полный доступ | Только к максимальному |
| **Удаление** | Любого элемента | Только максимального |
| **Внутренняя структура** | Дерево | Куча (heap) |
| **Сложность top()** | O(1) | O(1) |
| **Сложность pop()** | O(log n) | O(log n) |
| **Итерирование** | ✅ Возможно | ❌ Невозможно |

---

## 🎯 Заключение

### Ключевые преимущества s21::multiset:

✅ **Поддержка дубликатов** — естественное хранение повторяющихся данных  
✅ **Автоматическая сортировка** — все элементы упорядочены включая дубликаты  
✅ **Эффективный подсчет** — O(log n) для определения количества дубликатов  
✅ **Диапазонные операции** — удобная работа с группами одинаковых элементов  
✅ **Гибкость модификации** — можно удалить одно вхождение или все сразу  

### Идеальные случаи использования:

📊 **Частотный анализ** — подсчет количества вхождений элементов  
📈 **Статистические данные** — работа с повторяющимися измерениями  
🎵 **Приоритетные системы** — несколько задач с одинаковым приоритетом  
🏆 **Результаты соревнований** — учет одинаковых результатов  
📋 **Инвентарные системы** — подсчет количества однотипных предметов  
🎯 **Мультимножества** — математические операции с повторяющимися элементами  

### Ограничения:

❌ **Больше памяти** — каждый дубликат занимает место в дереве  
❌ **Сложность интерфейса** — больше методов по сравнению с set  
❌ **Неизменяемые элементы** — нельзя модифицировать элемент после вставки  

### Альтернативы:

- **s21::set** — если дубликаты не нужны
- **std::unordered_multiset** — для максимальной скорости без необходимости порядка
- **s21::map<Key, size_t>** — для хранения элемент → количество  
- **s21::vector + sort** — для редко изменяемых данных
- **std::priority_queue** — если нужна только max-heap функциональность

---

### 💡 Рекомендации по использованию

1. **Используйте multiset** когда нужна **сортировка + дубликаты + подсчет частоты**
2. **Предпочитайте equal_range()** для работы со всеми дубликатами одновременно
3. **Используйте count()** для быстрого получения количества вхождений
4. **Применяйте erase(iterator)** для удаления одного элемента, `erase(key)` для всех
5. **Рассмотрите unordered_multiset** если порядок не важен, а скорость критична

**s21::multiset — это мощный инструмент для статистического анализа и работы с повторяющимися отсортированными данными, идеальный выбор когда важны и порядок, и дубликаты! 🚀**

---

> 📝 **Примечание**: Данная документация основана на реализации s21::multiset в файле `src/source/headers/s21_multiset.h` проекта s21_containers. Для глубокого понимания внутренней структуры рекомендуется изучить [TREE.md](./TREE.md), [TREE-map.md](./TREE-map.md) и [TREE-set.md](./TREE-set.md).