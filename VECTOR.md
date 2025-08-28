# VECTOR - Динамический массив
## Детальный разбор внутренней реализации

### Оглавление
1. [Архитектура и принципы](#архитектура-и-принципы)
2. [Управление памятью](#управление-памятью)
3. [Стратегии роста capacity](#стратегии-роста-capacity)
4. [Итераторы произвольного доступа](#итераторы-произвольного-доступа)
5. [Операции вставки и удаления](#операции-вставки-и-удаления)
6. [Exception Safety](#exception-safety)
7. [Оптимизации и производительность](#оптимизации-и-производительность)
8. [Практические примеры](#практические-примеры)

---

## Архитектура и принципы

### Основная структура данных

```cpp
template <typename T>
class vector {
private:
    T* data_;           // Указатель на массив элементов
    size_type size_;    // Текущий размер (количество элементов)  
    size_type capacity_;// Размер выделенной памяти (в элементах)
};
```

### Концептуальная диаграмма

```
data_    size_=4    capacity_=8
 ↓
[A][B][C][D][?][?][?][?]  ← выделенная память
 ↑           ↑            ↑
begin()     end()        зарезервированное место
```

**Ключевые инварианты vector:**

1. `size_ <= capacity_` - всегда есть место для существующих элементов
2. `data_` указывает на валидный массив размером `capacity_` элементов (или nullptr при capacity_=0)
3. Элементы `[0, size_)` валидны и инициализированы
4. Элементы `[size_, capacity_)` - неинициализированная память

### Отличия от статического массива

| Аспект | array<T,N> | vector<T> |
|--------|------------|-----------|
| Размер | Фиксированный во время компиляции | Динамический во время выполнения |
| Память | Стек (обычно) | Куча |
| Емкость | Размер == емкость | Размер <= емкость |
| Перевыделение | Никогда | При превышении capacity |
| Накладные расходы | Нулевые | 24 байта + размер данных |

---

## Управление памятью

### Жизненный цикл памяти в vector

```cpp
class vector {
private:
    void reallocate(size_type new_capacity) {
        if (new_capacity == capacity_) return;
        
        T* new_data = nullptr;
        
        // 1. Выделяем новую память
        if (new_capacity > 0) {
            new_data = static_cast<T*>(::operator new(new_capacity * sizeof(T)));
        }
        
        // 2. Перемещаем/копируем существующие элементы
        size_type elements_to_move = std::min(size_, new_capacity);
        
        try {
            for (size_type i = 0; i < elements_to_move; ++i) {
                if constexpr (std::is_move_constructible_v<T>) {
                    // Используем move конструктор если возможно
                    new (new_data + i) T(std::move(data_[i]));
                } else {
                    // Fallback на copy конструктор
                    new (new_data + i) T(data_[i]);
                }
            }
        } catch (...) {
            // 3. Exception safety - уничтожаем частично созданные объекты
            for (size_type i = 0; i < elements_to_move; ++i) {
                (new_data + i)->~T();
            }
            ::operator delete(new_data);
            throw;
        }
        
        // 4. Уничтожаем старые объекты
        destroy_elements();
        
        // 5. Освобождаем старую память  
        ::operator delete(data_);
        
        // 6. Обновляем указатели
        data_ = new_data;
        capacity_ = new_capacity;
        size_ = elements_to_move;  // Может уменьшиться при shrink
    }
    
    void destroy_elements() {
        for (size_type i = 0; i < size_; ++i) {
            data_[i].~T();  // Вызываем деструктор каждого элемента
        }
    }
};
```

### Детали низкоуровневого управления памятью

**Использование placement new:**
```cpp
// Выделение сырой памяти без конструирования объектов
T* raw_memory = static_cast<T*>(::operator new(capacity * sizeof(T)));

// Конструирование объекта в заранее выделенной памяти  
new (raw_memory + i) T(args...);  // placement new

// Явный вызов деструктора (конструктор объект не удаляет память)
(raw_memory + i)->~T();

// Освобождение сырой памяти
::operator delete(raw_memory);
```

**Почему не используем new[] и delete[]:**
- `new[]` всегда вызывает конструктор по умолчанию для всех элементов
- vector должен контролировать, когда именно создавать объекты
- Нужна возможность создания объектов в произвольных позициях

### RAII и исключительная безопасность

```cpp
// Конструктор копирования с exception safety
vector(const vector& other) {
    data_ = nullptr;
    size_ = 0;
    capacity_ = 0;
    
    try {
        reserve(other.size_);  // Может бросить std::bad_alloc
        
        for (size_type i = 0; i < other.size_; ++i) {
            push_back(other.data_[i]);  // Может бросить исключение T::T()
        }
    } catch (...) {
        destroy_elements();     // Очищаем частично созданный вектор
        ::operator delete(data_);
        throw;                  // Перебрасываем исключение
    }
}
```

---

## Стратегии роста capacity

### Классические стратегии роста

```cpp
class vector {
private:
    void grow_capacity() {
        size_type new_capacity;
        
        if (capacity_ == 0) {
            new_capacity = 1;           // Начальный размер
        } else {
            // Стратегия геометрического роста
            new_capacity = capacity_ * 2;  // Множитель 2 (реально может быть 1.5 или 2)
            
            // Проверка на переполнение
            if (new_capacity < capacity_) { 
                new_capacity = max_size();
            }
        }
        
        reallocate(new_capacity);
    }
    
    void ensure_capacity(size_type min_capacity) {
        if (capacity_ >= min_capacity) return;
        
        size_type new_capacity = capacity_;
        while (new_capacity < min_capacity) {
            size_type old_capacity = new_capacity;
            new_capacity = (new_capacity == 0) ? 1 : new_capacity * 2;
            
            // Защита от переполнения
            if (new_capacity < old_capacity || new_capacity > max_size()) {
                new_capacity = max_size();
                break;
            }
        }
        
        reallocate(new_capacity);
    }
};
```

### Анализ различных коэффициентов роста

| Коэффициент | Преимущества | Недостатки | Использование |
|-------------|--------------|------------|---------------|
| 2.0 | Простота, быстрый рост | Больше неиспользуемой памяти | GNU libstdc++ |
| 1.5 | Лучшее использование памяти | Более сложные вычисления | MSVC, некоторые реализации |
| 1.618 (φ) | Оптимальное теоретически | Дробные вычисления | Академические реализации |

### Производительностный анализ стратегий

```cpp
// Анализ амортизированной сложности push_back

// При коэффициенте роста k > 1:
// Последовательность capacity: 1, k, k², k³, ...
// Общее количество копирований для n элементов:
// 1 + k + k² + ... + k^(log_k(n)) = (k^(log_k(n)+1) - 1)/(k-1) = O(n)

// Амортизированная сложность push_back: O(1)

void analyze_growth_performance() {
    vector<int> v;
    
    // При добавлении n элементов:
    for (int i = 0; i < n; ++i) {
        // Большинство вызовов: O(1) - просто копирование в конец
        // Редкие вызовы (когда capacity исчерпан): O(size) - перекопирование всех элементов
        v.push_back(i);  
    }
    
    // Общая сложность: O(n), амортизированно каждый push_back: O(1)
}
```

---

## Итераторы произвольного доступа

### Реализация Random Access Iterator

```cpp
class VectorIterator {
protected:
    T* ptr_;  // Просто указатель на элемент
    
public:
    // Все операции Random Access Iterator
    
    // 1. Базовые операции (как у всех итераторов)
    reference operator*() { return *ptr_; }
    T* operator->() { return ptr_; }
    
    // 2. Bidirectional операции
    VectorIterator& operator++() { ++ptr_; return *this; }
    VectorIterator& operator--() { --ptr_; return *this; }
    
    // 3. Random Access специфичные операции
    VectorIterator operator+(size_type n) const { 
        return VectorIterator(ptr_ + n); 
    }
    
    VectorIterator operator-(size_type n) const { 
        return VectorIterator(ptr_ - n); 
    }
    
    ptrdiff_t operator-(const VectorIterator& other) const { 
        return ptr_ - other.ptr_; 
    }
    
    VectorIterator& operator+=(size_type n) { 
        ptr_ += n; return *this; 
    }
    
    VectorIterator& operator-=(size_type n) { 
        ptr_ -= n; return *this; 
    }
    
    // 4. Сравнения (полный набор)
    bool operator==(const VectorIterator& other) const { return ptr_ == other.ptr_; }
    bool operator!=(const VectorIterator& other) const { return ptr_ != other.ptr_; }
    bool operator<(const VectorIterator& other) const { return ptr_ < other.ptr_; }
    bool operator>(const VectorIterator& other) const { return ptr_ > other.ptr_; }
    bool operator<=(const VectorIterator& other) const { return ptr_ <= other.ptr_; }
    bool operator>=(const VectorIterator& other) const { return ptr_ >= other.ptr_; }
};
```

### Преимущества итераторов vector

```cpp
void demonstrate_iterator_advantages() {
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 1. Random access - O(1)
    auto it = v.begin();
    auto element_5 = it + 4;  // Мгновенный переход к 5му элементу
    
    // 2. Арифметика итераторов
    auto distance = v.end() - v.begin();  // size() за O(1)
    
    // 3. Совместимость с STL алгоритмами
    std::sort(v.begin(), v.end());              // Требует Random Access
    std::binary_search(v.begin(), v.end(), 5);  // Требует Random Access
    
    // 4. Эффективная навигация
    auto middle = v.begin() + v.size()/2;       // O(1) переход к середине
    
    // Сравним с list (Bidirectional Iterator):
    std::list<int> l = {1, 2, 3, 4, 5};
    auto list_it = l.begin();
    std::advance(list_it, 2);  // O(n) переход к 3му элементу
    
    // list не поддерживает:
    // auto mid = l.begin() + l.size()/2;  // ОШИБКА КОМПИЛЯЦИИ!
    // std::sort(l.begin(), l.end());      // ОШИБКА КОМПИЛЯЦИИ!
}
```

### Invalidation правила для итераторов

```cpp
void iterator_invalidation_rules() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    auto it1 = v.begin();
    auto it2 = v.begin() + 2;
    
    // 1. push_back может инвалидировать ВСЕ итераторы при перевыделении
    v.push_back(6);  // it1, it2 могут быть невалидны!
    
    // 2. reserve() инвалидирует все итераторы при перевыделении
    v.reserve(100);  // it1, it2 точно невалидны
    
    // 3. insert() инвалидирует все итераторы после позиции вставки
    auto it3 = v.begin() + 1;
    v.insert(it3, 99);  // Итераторы после позиции 1 невалидны
    
    // 4. erase() инвалидирует итераторы после позиции удаления  
    v.erase(v.begin() + 1);  // Итераторы после позиции 1 невалидны
    
    // 5. Операции, НЕ инвалидирующие итераторы (если нет перевыделения):
    v[0] = 100;      // OK
    v.back() = 200;  // OK
    // pop_back() - инвалидирует только end() и итераторы на последний элемент
}
```

---

## Операции вставки и удаления

### Анализ операции insert

```cpp
iterator insert(iterator pos, const_reference value) {
    size_type index = pos - begin();  // Сохраняем позицию как индекс
    
    // 1. Проверяем, нужно ли увеличивать capacity
    if (size_ >= capacity_) {
        grow_capacity();  // Перевыделение инвалидирует pos!
        pos = begin() + index;  // Восстанавливаем позицию
    }
    
    // 2. Сдвигаем элементы вправо
    // Важно: движемся справа налево, чтобы не затирать данные
    for (size_type i = size_; i > index; --i) {
        if (i == size_) {
            // Создаем новый элемент в неинициализированной памяти
            new (data_ + i) T(std::move(data_[i-1]));
        } else {
            // Присваиваем в уже существующий объект
            data_[i] = std::move(data_[i-1]);
        }
    }
    
    // 3. Вставляем новый элемент
    if (index < size_) {
        data_[index] = value;  // Присваивание в существующий объект
    } else {
        new (data_ + index) T(value);  // Конструирование нового объекта
    }
    
    ++size_;
    return iterator(data_ + index);
}
```

### Визуализация вставки элемента

**Состояние до вставки:**
```
Вставляем 'X' в позицию 2

[A][B][C][D][_][_]  capacity=6, size=4
       ↑
     pos=2
```

**Пошаговый процесс:**
```
Шаг 1: Проверяем capacity (OK, есть место)

Шаг 2: Сдвигаем элементы
[A][B][C][C][D][_]  (D перемещен)
[A][B][C][C][C][_]  (C скопирован) 

Шаг 3: Вставляем новый элемент
[A][B][X][C][D][_]  size=5
       ↑
   результат
```

### Оптимизации операций

```cpp
// Оптимизированная реализация push_back
void push_back(const_reference value) {
    // Fast path - есть место
    if (size_ < capacity_) {
        new (data_ + size_) T(value);  // Placement new
        ++size_;
        return;
    }
    
    // Slow path - нужно перевыделение
    grow_capacity();
    new (data_ + size_) T(value);
    ++size_;
}

// Оптимизированная реализация emplace_back  
template<typename... Args>
void emplace_back(Args&&... args) {
    if (size_ < capacity_) {
        new (data_ + size_) T(std::forward<Args>(args)...);
        ++size_;
    } else {
        grow_capacity();
        new (data_ + size_) T(std::forward<Args>(args)...);
        ++size_;
    }
}
```

### Операции удаления

```cpp
// Удаление одного элемента
void erase(iterator pos) {
    size_type index = pos - begin();
    
    if (index >= size_) return;  // Защита от невалидного итератора
    
    // 1. Вызываем деструктор удаляемого элемента
    data_[index].~T();
    
    // 2. Сдвигаем элементы влево (затираем "дыру")
    for (size_type i = index; i < size_ - 1; ++i) {
        // Move assignment или copy assignment
        new (data_ + i) T(std::move(data_[i + 1]));
        data_[i + 1].~T();
    }
    
    --size_;
}

// Удаление диапазона элементов [first, last)
void erase(iterator first, iterator last) {
    size_type first_index = first - begin();
    size_type last_index = last - begin();
    size_type elements_to_remove = last_index - first_index;
    
    if (elements_to_remove == 0) return;
    
    // 1. Уничтожаем элементы в диапазоне
    for (size_type i = first_index; i < last_index; ++i) {
        data_[i].~T();
    }
    
    // 2. Сдвигаем оставшиеся элементы влево
    size_type elements_to_move = size_ - last_index;
    for (size_type i = 0; i < elements_to_move; ++i) {
        new (data_ + first_index + i) T(std::move(data_[last_index + i]));
        data_[last_index + i].~T();
    }
    
    size_ -= elements_to_remove;
}
```

---

## Exception Safety

### Уровни exception safety в vector

```cpp
class vector {
public:
    // 1. NOTHROW GUARANTEE - никогда не бросает исключения
    
    size_type size() const noexcept { return size_; }
    size_type capacity() const noexcept { return capacity_; }
    bool empty() const noexcept { return size_ == 0; }
    
    void swap(vector& other) noexcept {
        // Просто обмениваемся указателями - всегда безопасно
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }
    
    // 2. STRONG EXCEPTION SAFETY - состояние не изменяется при исключении
    
    void push_back(const T& value) {
        if (size_ < capacity_) {
            // Единственная операция, которая может бросить исключение
            new (data_ + size_) T(value);  // Может бросить T::T(const T&)
            ++size_;  // Увеличиваем размер только после успешного создания
        } else {
            // Более сложный случай - может потребоваться перевыделение
            push_back_with_reallocation(value);
        }
    }
    
private:
    void push_back_with_reallocation(const T& value) {
        vector temp(*this);         // Создаем копию текущего состояния
        temp.reserve(capacity_ * 2); // Может бросить std::bad_alloc
        temp.push_back(value);      // Может бросить T::T(const T&)
        
        // Если дошли сюда - все операции прошли успешно
        swap(temp);  // noexcept операция
        
        // Деструктор temp освободит старую память
    }
    
    // 3. BASIC EXCEPTION SAFETY - объект остается в валидном состоянии
    
public:
    vector& operator=(const vector& other) {
        if (this == &other) return *this;
        
        vector temp(other);  // Может бросить исключение
        swap(temp);          // noexcept - гарантированно не бросает
        return *this;        // temp автоматически уничтожится
        
        // Если конструктор копирования бросил исключение,
        // наш объект остался неизменным (strong safety)
    }
};
```

### Проблемы с исключениями при перевыделении

```cpp
void problematic_reallocation() {
    // Проблемная ситуация:
    vector<ComplexObject> v;
    
    // ComplexObject::ComplexObject(const ComplexObject&) может бросить исключение
    // При перевыделении:
    // 1. Выделяем новую память - OK
    // 2. Копируем элементы один за другим
    // 3. На элементе №5 конструктор копирования бросает исключение
    // 4. Что делать с частично скопированными элементами?
    
    v.push_back(ComplexObject{});  // Может вызвать перевыделение
}

// Решение - использование move semantics для exception safety
void move_based_reallocation() {
    // Если T имеет noexcept move конструктор:
    // 1. Перемещаем элементы вместо копирования
    // 2. Move операции обычно не бросают исключений
    // 3. Если бросают - помечаем как noexcept(false)
    
    if constexpr (std::is_nothrow_move_constructible_v<T>) {
        // Используем move - быстро и безопасно
        new (new_data + i) T(std::move(old_data[i]));
    } else {
        // Fallback на copy - медленно, но работает
        new (new_data + i) T(old_data[i]);
    }
}
```

---

## Оптимизации и производительность

### Оптимизации уровня компилятора

```cpp
// 1. Small String Optimization (SSO) принцип для vector
template<typename T, size_t SmallSize = 16/sizeof(T)>
class small_vector {
private:
    union {
        T* heap_data_;                    // Для больших векторов
        alignas(T) char stack_data_[SmallSize * sizeof(T)];  // Для малых
    };
    size_type size_;
    size_type capacity_;
    
    bool is_small() const { return capacity_ <= SmallSize; }
    
    T* data() {
        return is_small() ? reinterpret_cast<T*>(stack_data_) : heap_data_;
    }
};

// 2. Предвычисление capacity для известных паттернов
void optimized_construction() {
    // Плохо - много перевыделений
    vector<int> v1;
    for (int i = 0; i < 10000; ++i) {
        v1.push_back(i);  // Перевыделения на 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384
    }
    
    // Хорошо - одно выделение
    vector<int> v2;
    v2.reserve(10000);  // Выделяем сразу нужное количество
    for (int i = 0; i < 10000; ++i) {
        v2.push_back(i);  // Всегда O(1), никаких перевыделений
    }
}
```

### Оптимизации для специфических типов

```cpp
// Специализация для тривиальных типов
template<typename T>
void optimized_copy(T* dest, const T* src, size_t count) {
    if constexpr (std::is_trivially_copyable_v<T>) {
        // Для POD типов - быстрое копирование памяти
        std::memcpy(dest, src, count * sizeof(T));
    } else {
        // Для сложных типов - поэлементное копирование
        for (size_t i = 0; i < count; ++i) {
            new (dest + i) T(src[i]);
        }
    }
}

// Оптимизированное перемещение при перевыделении
void optimized_reallocation() {
    if constexpr (std::is_trivially_copyable_v<T>) {
        // POD типы можно просто скопировать битово
        std::memcpy(new_data, old_data, size_ * sizeof(T));
        // Деструкторы вызывать не нужно
    } else if constexpr (std::is_nothrow_move_constructible_v<T>) {
        // Move конструктор не бросает исключений - используем его
        for (size_t i = 0; i < size_; ++i) {
            new (new_data + i) T(std::move(old_data[i]));
        }
    } else {
        // Fallback на копирование с exception safety
        copy_with_exception_safety(new_data, old_data, size_);
    }
}
```

### Профилирование и бенчмарки

```cpp
#include <chrono>
#include <iostream>

void benchmark_vector_operations() {
    using namespace std::chrono;
    
    // Test 1: push_back vs reserve + push_back
    {
        auto start = high_resolution_clock::now();
        vector<int> v1;
        for (int i = 0; i < 1000000; ++i) {
            v1.push_back(i);
        }
        auto end = high_resolution_clock::now();
        std::cout << "Without reserve: " 
                  << duration_cast<milliseconds>(end - start).count() << "ms\n";
    }
    
    {
        auto start = high_resolution_clock::now();
        vector<int> v2;
        v2.reserve(1000000);
        for (int i = 0; i < 1000000; ++i) {
            v2.push_back(i);
        }
        auto end = high_resolution_clock::now();
        std::cout << "With reserve: " 
                  << duration_cast<milliseconds>(end - start).count() << "ms\n";
    }
    
    // Test 2: insert в середину vs push_back + sort
    // insert в середину: O(n) за каждую операцию = O(n²) общая сложность
    // push_back + sort: O(1) amortized + O(n log n) = O(n log n) общая сложность
}
```

---

## Практические примеры

### 1. Матричные вычисления с vector

```cpp
template<typename T>
class Matrix {
private:
    s21::vector<T> data_;
    size_t rows_, cols_;
    
public:
    Matrix(size_t rows, size_t cols) : rows_(rows), cols_(cols) {
        data_.resize(rows * cols);  // Выделяем память сразу
    }
    
    T& operator()(size_t row, size_t col) {
        return data_[row * cols_ + col];  // O(1) доступ
    }
    
    const T& operator()(size_t row, size_t col) const {
        return data_[row * cols_ + col];
    }
    
    // Эффективное матричное умножение
    Matrix operator*(const Matrix& other) const {
        Matrix result(rows_, other.cols_);
        
        // Используем row-major порядок для лучшей локальности
        for (size_t i = 0; i < rows_; ++i) {
            for (size_t k = 0; k < cols_; ++k) {
                T temp = (*this)(i, k);
                for (size_t j = 0; j < other.cols_; ++j) {
                    result(i, j) += temp * other(k, j);
                }
            }
        }
        return result;
    }
    
    // Транспонирование с минимальным количеством копирований
    Matrix transpose() const {
        Matrix result(cols_, rows_);
        
        // Block-wise транспонирование для лучшего кеширования
        const size_t block_size = 64;
        for (size_t ii = 0; ii < rows_; ii += block_size) {
            for (size_t jj = 0; jj < cols_; jj += block_size) {
                for (size_t i = ii; i < std::min(ii + block_size, rows_); ++i) {
                    for (size_t j = jj; j < std::min(jj + block_size, cols_); ++j) {
                        result(j, i) = (*this)(i, j);
                    }
                }
            }
        }
        return result;
    }
};

// Использование:
Matrix<double> m1(1000, 1000);
Matrix<double> m2(1000, 1000);

// Заполнение матриц...
auto result = m1 * m2;  // Эффективное умножение благодаря vector
```

### 2. Буфер для потокового ввода-вывода

```cpp
template<typename T>
class StreamBuffer {
private:
    s21::vector<T> buffer_;
    size_t read_pos_;
    size_t write_pos_;
    
public:
    StreamBuffer(size_t initial_capacity = 1024) 
        : read_pos_(0), write_pos_(0) {
        buffer_.reserve(initial_capacity);
    }
    
    // Добавление данных в буфер
    void write(const T* data, size_t count) {
        // Проверяем, достаточно ли места
        if (write_pos_ + count > buffer_.capacity()) {
            // Сдвигаем данные к началу, если есть свободное место в начале
            if (read_pos_ > 0 && 
                (write_pos_ - read_pos_ + count) <= buffer_.capacity()) {
                compact();
            } else {
                // Увеличиваем буфер
                size_t new_size = std::max(buffer_.capacity() * 2, 
                                         write_pos_ + count);
                buffer_.reserve(new_size);
            }
        }
        
        // Копируем данные
        for (size_t i = 0; i < count; ++i) {
            if (write_pos_ >= buffer_.size()) {
                buffer_.push_back(data[i]);
            } else {
                buffer_[write_pos_] = data[i];
            }
            ++write_pos_;
        }
    }
    
    // Чтение данных из буфера
    size_t read(T* data, size_t max_count) {
        size_t available = write_pos_ - read_pos_;
        size_t to_read = std::min(max_count, available);
        
        for (size_t i = 0; i < to_read; ++i) {
            data[i] = buffer_[read_pos_ + i];
        }
        
        read_pos_ += to_read;
        return to_read;
    }
    
private:
    void compact() {
        size_t data_size = write_pos_ - read_pos_;
        
        // Перемещаем данные к началу буфера
        for (size_t i = 0; i < data_size; ++i) {
            buffer_[i] = std::move(buffer_[read_pos_ + i]);
        }
        
        read_pos_ = 0;
        write_pos_ = data_size;
    }
};
```

### 3. Пул объектов с vector

```cpp
template<typename T>
class ObjectPool {
private:
    s21::vector<T> objects_;           // Все объекты
    s21::vector<size_t> free_indices_; // Индексы свободных объектов
    s21::vector<bool> is_free_;        // Быстрая проверка, свободен ли объект
    
public:
    ObjectPool(size_t initial_size = 100) {
        objects_.reserve(initial_size);
        free_indices_.reserve(initial_size);
        is_free_.reserve(initial_size);
        
        // Создаем начальный пул объектов
        for (size_t i = 0; i < initial_size; ++i) {
            objects_.emplace_back();  // Создаем объект T на месте
            free_indices_.push_back(i);
            is_free_.push_back(true);
        }
    }
    
    // Получение объекта из пула
    T* acquire() {
        if (free_indices_.empty()) {
            // Расширяем пул
            size_t old_size = objects_.size();
            size_t new_size = old_size * 2;
            
            objects_.resize(new_size);
            is_free_.resize(new_size, true);
            
            for (size_t i = old_size; i < new_size; ++i) {
                free_indices_.push_back(i);
            }
        }
        
        size_t index = free_indices_.back();
        free_indices_.pop_back();
        is_free_[index] = false;
        
        return &objects_[index];
    }
    
    // Возврат объекта в пул
    void release(T* obj) {
        // Находим индекс объекта
        size_t index = obj - objects_.data();
        
        if (index < objects_.size() && !is_free_[index]) {
            is_free_[index] = true;
            free_indices_.push_back(index);
            
            // Опционально: сбрасываем состояние объекта
            obj->reset();
        }
    }
    
    // Статистика пула
    size_t total_objects() const { return objects_.size(); }
    size_t free_objects() const { return free_indices_.size(); }
    size_t used_objects() const { return total_objects() - free_objects(); }
};

// Использование с пулом соединений к БД
class DatabaseConnection {
public:
    void reset() { /* сбрасываем состояние соединения */ }
    void execute_query(const std::string& query) { /* выполняем запрос */ }
};

ObjectPool<DatabaseConnection> connection_pool(10);

void handle_request() {
    auto* conn = connection_pool.acquire();
    try {
        conn->execute_query("SELECT * FROM users");
    } catch (...) {
        connection_pool.release(conn);
        throw;
    }
    connection_pool.release(conn);
}
```

### 4. Временные ряды и скользящие окна

```cpp
template<typename T>
class TimeSeriesBuffer {
private:
    s21::vector<std::pair<std::chrono::steady_clock::time_point, T>> data_;
    std::chrono::milliseconds window_size_;
    
public:
    TimeSeriesBuffer(std::chrono::milliseconds window_size, size_t initial_capacity = 1000)
        : window_size_(window_size) {
        data_.reserve(initial_capacity);
    }
    
    void add_sample(const T& value) {
        auto now = std::chrono::steady_clock::now();
        data_.emplace_back(now, value);
        
        // Удаляем старые данные за пределами окна
        cleanup_old_data(now);
    }
    
    // Вычисление скользящего среднего
    double moving_average() const {
        if (data_.empty()) return 0.0;
        
        double sum = 0.0;
        for (const auto& [timestamp, value] : data_) {
            sum += static_cast<double>(value);
        }
        return sum / data_.size();
    }
    
    // Вычисление максимума в окне
    T max_in_window() const {
        if (data_.empty()) return T{};
        
        T max_val = data_[0].second;
        for (size_t i = 1; i < data_.size(); ++i) {
            if (data_[i].second > max_val) {
                max_val = data_[i].second;
            }
        }
        return max_val;
    }
    
    // Квантили (требует сортировки)
    T quantile(double p) const {
        if (data_.empty()) return T{};
        
        // Создаем копию значений для сортировки
        s21::vector<T> values;
        values.reserve(data_.size());
        
        for (const auto& [timestamp, value] : data_) {
            values.push_back(value);
        }
        
        std::sort(values.begin(), values.end());
        
        size_t index = static_cast<size_t>(p * (values.size() - 1));
        return values[index];
    }
    
private:
    void cleanup_old_data(std::chrono::steady_clock::time_point current_time) {
        auto threshold = current_time - window_size_;
        
        // Находим первый элемент, который еще актуален
        auto it = std::lower_bound(data_.begin(), data_.end(), 
                                 std::make_pair(threshold, T{}),
                                 [](const auto& a, const auto& b) {
                                     return a.first < b.first;
                                 });
        
        // Удаляем все элементы до найденного
        if (it != data_.begin()) {
            data_.erase(data_.begin(), it);
        }
    }
};

// Использование для мониторинга производительности
TimeSeriesBuffer<double> cpu_usage(std::chrono::seconds(60));  // 1 минутное окно

void monitor_cpu() {
    while (true) {
        double current_cpu = get_cpu_usage();  // Получаем текущую загрузку CPU
        cpu_usage.add_sample(current_cpu);
        
        // Анализируем данные
        double avg = cpu_usage.moving_average();
        double p95 = cpu_usage.quantile(0.95);
        
        if (avg > 80.0 || p95 > 95.0) {
            alert("High CPU usage detected!");
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
```

---

## Заключение

Vector в реализации s21 представляет собой фундаментальную структуру данных, демонстрирующую многие ключевые концепции современного C++:

### Архитектурные преимущества:

1. **Произвольный доступ O(1)** - быстрый доступ к элементам по индексу
2. **Локальность данных** - элементы расположены непрерывно в памяти, оптимально для CPU cache
3. **Амортизированная O(1) вставка в конец** - эффективный рост благодаря геометрическому увеличению capacity
4. **Совместимость с STL алгоритмами** - Random Access Iterator позволяет использовать все алгоритмы

### Ключевые реализационные детали:

1. **Низкоуровневое управление памятью** - использование `operator new/delete` вместо `new[]/delete[]` для точного контроля жизненного цикла объектов
2. **Exception Safety** - гарантии strong/basic exception safety через RAII и copy-and-swap идиому
3. **Move semantics** - эффективные операции перемещения, особенно при перевыделении памяти
4. **Template оптимизации** - специализация поведения в зависимости от свойств типа T

### Компромиссы и ограничения:

- **Дорогие операции вставки/удаления в середине** - O(n) из-за необходимости сдвига элементов
- **Возможные перевыделения** - инвалидация итераторов при росте capacity
- **Память может фрагментироваться** - при частых изменениях размера
- **Накладные расходы на управление capacity** - может использовать в 2 раза больше памяти чем нужно

Vector остается одной из наиболее часто используемых структур данных в C++ благодаря оптимальному сочетанию производительности, простоты использования и широкой применимости для большинства задач, где требуется динамический массив.