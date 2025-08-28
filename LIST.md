# LIST - Двусвязный список
## Детальный разбор внутренней реализации

### Оглавление
1. [Общая архитектура](#общая-архитектура)
2. [Структура узла Node](#структура-узла-node)
3. [Sentinel-узел и организация памяти](#sentinel-узел-и-организация-памяти)
4. [Итераторы и навигация](#итераторы-и-навигация)
5. [Операции вставки и удаления](#операции-вставки-и-удаления)
6. [Сложные операции](#сложные-операции)
7. [Управление памятью](#управление-памятью)
8. [Практические примеры](#практические-примеры)

---

## Общая архитектура

### Концептуальная схема двусвязного списка

```cpp
template <typename T>
class list {
private:
    struct Node {
        T data;              // Пользовательские данные
        Node* next;          // Указатель на следующий узел  
        Node* prev;          // Указатель на предыдущий узел
    };
    
    Node* head_;      // Указатель на первый элемент
    Node* tail_;      // Указатель на последний элемент
    Node* sentinel_;  // Специальный sentinel-узел 
    size_type size_;  // Количество элементов
};
```

### Визуальная диаграмма структуры

```
[sentinel_] ←→ [Node1] ←→ [Node2] ←→ [Node3] ←→ [sentinel_]
    ↑             ↑                      ↑
    |           head_                  tail_
    |
   end()
```

**Ключевые особенности архитектуры:**

1. **Двунаправленные связи**: Каждый узел содержит указатели на предыдущий (`prev`) и следующий (`next`) узлы
2. **Циклическая структура**: Последний узел указывает на `sentinel_`, а `sentinel_` указывает обратно на первый
3. **Sentinel-узел**: Специальный узел, который упрощает реализацию итераторов и граничных случаев

---

## Структура узла Node

### Анализ реализации Node

```cpp
struct Node {
    value_type data;          // Данные узла
    Node* next;               // Указатель на следующий узел
    Node* prev;               // Указатель на предыдущий узел
    
    // Конструктор по умолчанию - создает пустой узел
    Node() : data(value_type()), next(nullptr), prev(nullptr) {}
    
    // Конструктор с данными - инициализирует только data
    explicit Node(const_reference value) 
        : data(value), next(nullptr), prev(nullptr) {}
    
    // Полный конструктор - устанавливает данные и связи
    Node(const_reference value, Node* next_ptr, Node* prev_ptr) 
        : data(value), next(next_ptr), prev(prev_ptr) {}
};
```

### Детали реализации конструкторов Node

**1. Конструктор по умолчанию:**
```cpp
Node() : data(value_type()), next(nullptr), prev(nullptr) {}
```
- `value_type()` - вызывает конструктор по умолчанию для типа T
- Для `int` это будет `0`, для `std::string` - пустая строка
- Указатели инициализируются как `nullptr`

**2. Explicit конструктор:**
```cpp
explicit Node(const_reference value) : data(value), next(nullptr), prev(nullptr) {}
```
- `explicit` предотвращает неявные преобразования
- Принимает значение по константной ссылке (избегаем копирования)
- Связи устанавливаются отдельно после создания

**3. Полный конструктор:**
```cpp
Node(const_reference value, Node* next_ptr, Node* prev_ptr) 
    : data(value), next(next_ptr), prev(prev_ptr) {}
```
- Устанавливает все поля сразу
- Используется в сложных операциях вставки

### Компактность и выравнивание памяти

```cpp
// Для Node<int> размер будет примерно:
sizeof(Node<int>) = sizeof(int) + 2 * sizeof(void*) + padding
// На 64-битной системе: 4 + 16 + 4 = 24 байта (с учетом выравнивания)

// Для Node<std::string>:
sizeof(Node<std::string>) = sizeof(std::string) + 2 * sizeof(void*) + padding  
// На 64-битной системе: 32 + 16 = 48 байтов
```

---

## Sentinel-узел и организация памяти

### Роль и назначение sentinel_

Sentinel-узел - это ключевая архитектурная особенность, которая радикально упрощает реализацию двусвязного списка.

```cpp
// Состояние пустого списка
[sentinel_]
    ↑
head_ = tail_ = sentinel_

// Состояние списка с элементами  
[sentinel_] ←→ [Node1] ←→ [Node2] ←→ [sentinel_]
    ↑             ↑          ↑
    |           head_      tail_
    |
   end()
```

### Преимущества использования sentinel

**1. Упрощение граничных случаев:**
```cpp
// Без sentinel - нужны проверки на nullptr
void push_back(const T& value) {
    Node* new_node = new Node(value);
    if (tail_ == nullptr) {        // Пустой список
        head_ = tail_ = new_node;
    } else {                       // Непустой список
        tail_->next = new_node;
        new_node->prev = tail_;
        tail_ = new_node;
    }
}

// С sentinel - единообразный код
void push_back(const T& value) {
    Node* new_node = new Node(value, sentinel_, sentinel_->prev);
    sentinel_->prev->next = new_node;  // Всегда работает!
    sentinel_->prev = new_node;
    if (head_ == sentinel_) head_ = new_node;  // Только для первого элемента
}
```

**2. Безопасность итераторов:**
```cpp
// end() всегда указывает на sentinel_
iterator end() { return iterator(sentinel_); }

// Декремент end() дает last element
iterator it = my_list.end();
--it;  // Теперь it указывает на последний реальный элемент
```

### Инициализация sentinel-узла

```cpp
void init_empty_list() {
    sentinel_ = new Node();           // Создаем sentinel
    sentinel_->next = sentinel_;      // Замыкаем цикл
    sentinel_->prev = sentinel_;
    head_ = tail_ = sentinel_;        // В пустом списке head_/tail_ указывают на sentinel_
    size_ = 0;
}
```

---

## Итераторы и навигация

### Архитектура итераторов

```cpp
class ListIterator {
protected:
    Node* node_;  // Указатель на текущий узел
    
public:
    explicit ListIterator(Node* node) : node_(node) {}
    
    reference operator*() { return node_->data; }
    
    // Префиксный инкремент - переход к следующему узлу
    ListIterator& operator++() {
        node_ = node_->next;
        return *this;
    }
    
    // Постфиксный инкремент 
    ListIterator operator++(int) {
        ListIterator temp = *this;
        ++(*this);
        return temp;
    }
};
```

### Детали реализации операций итератора

**1. Разыменование (`operator*`):**
```cpp
reference operator*() { 
    // ВНИМАНИЕ: нет проверки на sentinel_!
    // Разыменование end() итератора - undefined behavior
    return node_->data; 
}
```

**2. Инкремент и декремент:**
```cpp
// Префиксный инкремент: ++it
ListIterator& operator++() {
    node_ = node_->next;    // Переходим к следующему узлу
    return *this;           // Возвращаем ссылку на себя
}

// Постфиксный инкремент: it++
ListIterator operator++(int) {
    ListIterator temp = *this;  // Сохраняем старое значение
    ++(*this);                  // Используем префиксную версию
    return temp;                // Возвращаем старое значение
}
```

### Константные итераторы

```cpp
class ListConstIterator : public ListIterator {
public:
    // Наследует все от ListIterator, но переопределяет разыменование
    const_reference operator*() const {
        return node_->data;  // Возвращает const ссылку
    }
    
    // Конструктор преобразования из обычного итератора
    ListConstIterator(const ListIterator& other) : ListIterator(other) {}
};
```

### Безопасность итераторов и sentinel

```cpp
// Благодаря sentinel, итераторы всегда валидны для навигации
for (auto it = list.begin(); it != list.end(); ++it) {
    // it никогда не станет nullptr
    // end() всегда валиден для сравнения
    std::cout << *it << " ";
}

// Обратная итерация тоже безопасна
auto it = list.end();
while (it != list.begin()) {
    --it;  // Декремент end() даст последний реальный элемент
    std::cout << *it << " ";
}
```

---

## Операции вставки и удаления

### Анализ операции insert

```cpp
iterator insert(iterator pos, const_reference value) {
    // pos может указывать на любой узел, включая sentinel_ (end())
    Node* current = pos.node_;
    
    // Создаем новый узел, сразу устанавливая связи
    Node* new_node = new Node(value, current, current->prev);
    
    // Обновляем связи соседних узлов
    current->prev->next = new_node;  // Предыдущий узел теперь указывает на новый
    current->prev = new_node;        // Текущий узел указывает назад на новый
    
    // Обновляем head_ если вставляем в начало
    if (current == head_) {
        head_ = new_node;
    }
    
    ++size_;
    return iterator(new_node);
}
```

### Визуализация вставки

**До вставки:**
```
[A] ←→ [B] ←→ [C]
       ↑
      pos
```

**После вставки нового элемента X:**
```
[A] ←→ [X] ←→ [B] ←→ [C]
       ↑
   новый узел
```

**Пошаговый процесс:**
1. `new_node = new Node(X, B, A)` - создаем узел X с связями на B и A
2. `A->next = new_node` - A теперь указывает на X
3. `B->prev = new_node` - B теперь указывает назад на X

### Операция erase

```cpp
void erase(iterator pos) {
    Node* to_delete = pos.node_;
    
    // Нельзя удалять sentinel_
    if (to_delete == sentinel_) {
        return;  // Или бросить исключение
    }
    
    // Обновляем связи соседних узлов
    to_delete->prev->next = to_delete->next;
    to_delete->next->prev = to_delete->prev;
    
    // Обновляем head_/tail_ если нужно
    if (to_delete == head_) {
        head_ = (to_delete->next == sentinel_) ? sentinel_ : to_delete->next;
    }
    if (to_delete == tail_) {
        tail_ = (to_delete->prev == sentinel_) ? sentinel_ : to_delete->prev;
    }
    
    delete to_delete;
    --size_;
}
```

### Push/Pop операции

**Push_back реализация:**
```cpp
void push_back(const_reference value) {
    // Вставляем перед sentinel_ (в конец списка)
    insert(iterator(sentinel_), value);
}

// Или прямая реализация:
void push_back(const_reference value) {
    Node* new_node = new Node(value, sentinel_, sentinel_->prev);
    
    // Обновляем связи
    sentinel_->prev->next = new_node;
    sentinel_->prev = new_node;
    
    // Обновляем tail_
    tail_ = new_node;
    
    // Если это первый элемент
    if (head_ == sentinel_) {
        head_ = new_node;
    }
    
    ++size_;
}
```

---

## Сложные операции

### Splice - перенос элементов между списками

Splice - одна из самых мощных и сложных операций list, которая позволяет переносить элементы между списками за O(1) или O(n) время.

```cpp
// Перенос всего списка other в позицию pos
void splice(const_iterator pos, list& other) {
    if (other.empty() || &other == this) return;
    
    Node* pos_node = pos.node_;
    Node* other_first = other.head_;
    Node* other_last = other.tail_;
    
    // Отсоединяем элементы из other
    other.head_ = other.tail_ = other.sentinel_;
    other.sentinel_->next = other.sentinel_->prev = other.sentinel_;
    
    // Вставляем в текущий список
    other_last->next = pos_node;
    other_first->prev = pos_node->prev;
    pos_node->prev->next = other_first;
    pos_node->prev = other_last;
    
    // Обновляем head_/tail_ если нужно
    if (pos_node == head_) head_ = other_first;
    if (pos == end()) tail_ = other_last;
    
    // Обновляем размеры
    size_ += other.size_;
    other.size_ = 0;
}
```

### Merge - объединение отсортированных списков

```cpp
void merge(list& other) {
    if (&other == this || other.empty()) return;
    
    iterator it1 = begin();
    iterator it2 = other.begin();
    
    while (it1 != end() && it2 != other.end()) {
        if (*it2 < *it1) {
            // Переносим элемент из other перед it1
            iterator next_it2 = it2;
            ++next_it2;
            
            // Splice операция для одного элемента
            splice(it1, other, it2);
            it2 = next_it2;
        } else {
            ++it1;
        }
    }
    
    // Переносим оставшиеся элементы из other
    if (it2 != other.end()) {
        splice(end(), other, it2, other.end());
    }
}
```

### Sort - сортировка merge sort

```cpp
void sort() {
    if (size() <= 1) return;
    
    // Используем merge sort, адаптированный для списков
    merge_sort_recursive(head_, size());
}

private:
Node* merge_sort_recursive(Node* start, size_type n) {
    if (n <= 1) return start;
    
    // Разделяем список пополам
    size_type half = n / 2;
    Node* mid = start;
    for (size_type i = 0; i < half; ++i) {
        mid = mid->next;
    }
    
    // Рекурсивно сортируем половины  
    Node* left = merge_sort_recursive(start, half);
    Node* right = merge_sort_recursive(mid, n - half);
    
    // Объединяем отсортированные половины
    return merge_sorted_sublists(left, right, half, n - half);
}
```

---

## Управление памятью

### RAII и исключительная безопасность

```cpp
// Деструктор должен быть exception-safe
~list() {
    clear();                    // Удаляем все элементы
    delete sentinel_;           // Удаляем sentinel
}

// Clear с гарантией отсутствия исключений
void clear() noexcept {
    Node* current = head_;
    while (current != sentinel_) {
        Node* next = current->next;
        delete current;         // ~T() может бросить исключение!
        current = next;
    }
    
    // Восстанавливаем пустое состояние
    head_ = tail_ = sentinel_;
    sentinel_->next = sentinel_->prev = sentinel_;
    size_ = 0;
}
```

### Конструктор копирования

```cpp
list(const list& other) {
    init_empty_list();          // Создаем пустой список
    
    try {
        for (const auto& item : other) {
            push_back(item);    // Может бросить исключение
        }
    } catch (...) {
        clear();                // Очищаем частично созданный список
        delete sentinel_;
        throw;                  // Пробрасываем исключение
    }
}
```

### Move семантика

```cpp
list(list&& other) noexcept {
    // Просто крадем внутреннее состояние
    head_ = other.head_;
    tail_ = other.tail_;
    sentinel_ = other.sentinel_;
    size_ = other.size_;
    
    // Оставляем other в валидном пустом состоянии
    other.init_empty_list();
}

list& operator=(list&& other) noexcept {
    if (this != &other) {
        clear();                // Очищаем текущее содержимое
        delete sentinel_;
        
        // Перемещаем состояние
        head_ = other.head_;
        tail_ = other.tail_;
        sentinel_ = other.sentinel_;
        size_ = other.size_;
        
        other.init_empty_list();
    }
    return *this;
}
```

---

## Практические примеры

### 1. Реализация LRU Cache

```cpp
#include "s21_list.h"
#include <unordered_map>

template<typename Key, typename Value>
class LRUCache {
private:
    struct CacheItem {
        Key key;
        Value value;
        CacheItem(const Key& k, const Value& v) : key(k), value(v) {}
    };
    
    s21::list<CacheItem> items_;
    std::unordered_map<Key, typename s21::list<CacheItem>::iterator> index_;
    size_t capacity_;
    
public:
    LRUCache(size_t capacity) : capacity_(capacity) {}
    
    Value* get(const Key& key) {
        auto it = index_.find(key);
        if (it == index_.end()) return nullptr;
        
        // Перемещаем элемент в начало (самый недавно использованный)
        auto list_it = it->second;
        items_.splice(items_.begin(), items_, list_it);
        
        return &list_it->value;
    }
    
    void put(const Key& key, const Value& value) {
        auto it = index_.find(key);
        
        if (it != index_.end()) {
            // Обновляем существующий элемент
            it->second->value = value;
            items_.splice(items_.begin(), items_, it->second);
        } else {
            // Добавляем новый элемент
            if (items_.size() >= capacity_) {
                // Удаляем самый старый элемент
                auto last = items_.back();
                index_.erase(last.key);
                items_.pop_back();
            }
            
            items_.push_front(CacheItem(key, value));
            index_[key] = items_.begin();
        }
    }
};

// Использование:
LRUCache<int, std::string> cache(3);
cache.put(1, "one");
cache.put(2, "two");
cache.put(3, "three");

std::string* val = cache.get(1);  // "one", элемент 1 становится самым новым
cache.put(4, "four");             // Элемент 2 удаляется как самый старый
```

### 2. Реализация Undo/Redo системы

```cpp
template<typename Command>
class UndoRedoManager {
private:
    s21::list<Command> history_;
    typename s21::list<Command>::iterator current_;
    
public:
    UndoRedoManager() {
        current_ = history_.end();
    }
    
    void execute(const Command& cmd) {
        // Удаляем всю "будущую" историю после текущей позиции
        history_.erase(current_, history_.end());
        
        // Добавляем новую команду
        history_.push_back(cmd);
        current_ = history_.end();
        
        // Выполняем команду
        cmd.execute();
    }
    
    bool undo() {
        if (current_ == history_.begin()) return false;
        
        --current_;
        current_->undo();
        return true;
    }
    
    bool redo() {
        if (current_ == history_.end()) return false;
        
        current_->execute();
        ++current_;
        return true;
    }
};
```

### 3. Музыкальный плейлист с продвинутой навигацией

```cpp
class MusicPlaylist {
private:
    struct Song {
        std::string title;
        std::string artist;
        int duration_seconds;
        
        Song(const std::string& t, const std::string& a, int d) 
            : title(t), artist(a), duration_seconds(d) {}
    };
    
    s21::list<Song> playlist_;
    typename s21::list<Song>::iterator current_song_;
    bool shuffle_mode_;
    
public:
    void add_song(const std::string& title, const std::string& artist, int duration) {
        playlist_.push_back(Song(title, artist, duration));
        if (playlist_.size() == 1) {
            current_song_ = playlist_.begin();
        }
    }
    
    void next_song() {
        if (playlist_.empty()) return;
        
        if (shuffle_mode_) {
            // В реальности тут был бы более сложный алгоритм shuffle
            ++current_song_;
            if (current_song_ == playlist_.end()) {
                current_song_ = playlist_.begin();
            }
        } else {
            ++current_song_;
            if (current_song_ == playlist_.end()) {
                current_song_ = playlist_.begin();  // Зацикливание
            }
        }
    }
    
    void previous_song() {
        if (playlist_.empty()) return;
        
        if (current_song_ == playlist_.begin()) {
            current_song_ = playlist_.end();
            --current_song_;  // Переходим к последней песне
        } else {
            --current_song_;
        }
    }
    
    // Перестановка песен в плейлисте
    void move_song_up(typename s21::list<Song>::iterator it) {
        if (it != playlist_.begin()) {
            auto prev_it = it;
            --prev_it;
            playlist_.splice(prev_it, playlist_, it);
        }
    }
    
    void move_song_down(typename s21::list<Song>::iterator it) {
        auto next_it = it;
        ++next_it;
        if (next_it != playlist_.end()) {
            ++next_it;  // Перемещаем после следующего элемента
            playlist_.splice(next_it, playlist_, it);
        }
    }
};
```

### 4. Обработка больших файлов построчно

```cpp
class LineBuffer {
private:
    s21::list<std::string> lines_;
    const size_t MAX_LINES = 1000;  // Ограничение памяти
    
public:
    void process_file(const std::string& filename) {
        std::ifstream file(filename);
        std::string line;
        
        while (std::getline(file, line)) {
            lines_.push_back(line);
            
            // Если буфер переполнен, обрабатываем и очищаем
            if (lines_.size() >= MAX_LINES) {
                process_batch();
                lines_.clear();
            }
        }
        
        // Обрабатываем оставшиеся строки
        if (!lines_.empty()) {
            process_batch();
        }
    }
    
private:
    void process_batch() {
        // Обрабатываем строки, используя преимущества list
        // для эффективных вставок/удалений
        
        auto it = lines_.begin();
        while (it != lines_.end()) {
            if (should_filter_line(*it)) {
                it = lines_.erase(it);  // erase возвращает следующий итератор
            } else {
                transform_line(*it);
                ++it;
            }
        }
        
        // Отправляем обработанные строки дальше
        output_batch(lines_);
    }
};
```

---

## Заключение

Двусвязный список в реализации s21::list демонстрирует множество продвинутых концепций C++:

### Ключевые архитектурные решения:
1. **Sentinel-узел** - радикально упрощает реализацию и повышает безопасность итераторов
2. **RAII** - автоматическое управление памятью через деструкторы
3. **Move семантика** - эффективные операции перемещения без копирования
4. **Template метапrogramming** - универсальность для любых типов

### Преимущества двусвязного списка:
- **O(1)** вставка/удаление в любой позиции (при наличии итератора)  
- **O(1)** splice операции для перестановки элементов
- **Стабильность итераторов** - итераторы остаются валидными при модификациях
- **Отсутствие перевыделений** - каждый элемент хранится независимо

### Недостатки:
- **Нет произвольного доступа** - O(n) для доступа к элементу по индексу
- **Накладные расходы памяти** - каждый элемент требует дополнительно 16 байт для указателей
- **Плохая локальность данных** - элементы разбросаны по памяти

Эта реализация показывает, как грамотное использование указателей, RAII принципов и современных возможностей C++ позволяет создать эффективную и безопасную структуру данных, которая является основой для многих алгоритмов и паттернов программирования.