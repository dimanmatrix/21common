# ARRAY - Статический массив
## Детальный разбор внутренней реализации

### Оглавление
1. [Концепция статического контейнера](#концепция-статического-контейнера)
2. [Template параметры и их роль](#template-параметры-и-их-роль)
3. [Compile-time вычисления](#compile-time-вычисления)
4. [Управление памятью](#управление-памяти)
5. [Итераторы как указатели](#итераторы-как-указатели)
6. [Zero-size array проблема](#zero-size-array-проблема)
7. [Оптимизации компилятора](#оптимизации-компилятора)
8. [Практические примеры](#практические-примеры)

---

## Концепция статического контейнера

### Философия array в сравнении с динамическими контейнерами

```cpp
template <typename T, size_t N>
class array {
private:
    value_type data_[N > 0 ? N : 1];  // Статический C-массив
    // НЕТ: size_, capacity_, указателей на динамическую память
};
```

**Принципиальные отличия:**

| Аспект | vector<T> | array<T,N> |
|--------|-----------|------------|
| Размер | Динамический (runtime) | Фиксированный (compile-time) |
| Память | Куча (`new`/`malloc`) | Стек (или где создан объект) |
| Накладные расходы | 24 байта + данные | 0 байт + данные |
| Итераторы | Класс-обертка | Простые указатели |
| Безопасность | Проверка границ в `at()` | Проверка границ в `at()` |
| Перевыделение | Возможно | Никогда |

### Архитектурная диаграмма

```cpp
// Объект array<int, 5> в памяти:
array<int, 5> arr;

Стек:
[arr.data_[0]] = int
[arr.data_[1]] = int  
[arr.data_[2]] = int
[arr.data_[3]] = int
[arr.data_[4]] = int

// Никаких указателей, размеров, capacity - только данные!
// sizeof(array<int, 5>) == 5 * sizeof(int) == 20 байт
```

---

## Template параметры и их роль

### Анализ template параметров

```cpp
template <typename T, size_t N>
//           ^           ^
//           |           |
//      Тип элемента   Размер массива
class array {
```

**Детали параметра `size_t N`:**

```cpp
// N - это compile-time константа
// Компилятор должен знать её значение во время компиляции

// ✅ РАБОТАЕТ - компилятор знает значение
constexpr size_t SIZE = 10;
array<int, SIZE> arr1;
array<int, 42> arr2;
array<int, sizeof(double)> arr3;

// ❌ НЕ РАБОТАЕТ - runtime значение
int runtime_size = 10;
array<int, runtime_size> arr4;  // ОШИБКА КОМПИЛЯЦИИ

// ✅ РАБОТАЕТ - constexpr функция
constexpr size_t calculate_size() { return 20; }
array<int, calculate_size()> arr5;
```

### Template специализации и SFINAE

```cpp
// Специализация для массива нулевого размера
template <typename T>
class array<T, 0> {
public:
    constexpr bool empty() const noexcept { return true; }
    constexpr size_type size() const noexcept { return 0; }
    
    // Специальное поведение для итераторов
    constexpr iterator begin() noexcept { return nullptr; }
    constexpr iterator end() noexcept { return nullptr; }
    
    // at() и operator[] всегда бросают исключение
    reference at(size_type pos) { 
        throw std::out_of_range("array<T, 0>::at"); 
    }
};

// Использование SFINAE для условной компиляции методов
template <typename T, size_t N>
class array {
public:
    // Этот метод доступен только если N > 0
    template<size_t Size = N>
    typename std::enable_if<Size != 0, reference>::type
    front() noexcept {
        return data_[0];
    }
    
    // Альтернативная версия с C++17 if constexpr
    reference front() noexcept {
        if constexpr (N > 0) {
            return data_[0];
        } else {
            static_assert(N > 0, "front() called on empty array");
        }
    }
};
```

---

## Compile-time вычисления

### Constexpr методы и их значение

```cpp
template <typename T, size_t N>
class array {
public:
    // Эти методы можно вызвать во время компиляции
    constexpr bool empty() const noexcept { 
        return N == 0; 
    }
    
    constexpr size_type size() const noexcept { 
        return N; 
    }
    
    constexpr size_type max_size() const noexcept { 
        return N; 
    }
    
    // Доступ к элементам тоже может быть constexpr (с C++14)
    constexpr reference operator[](size_type pos) noexcept {
        return data_[pos];
    }
    
    constexpr const_reference operator[](size_type pos) const noexcept {
        return data_[pos];
    }
};
```

### Практическое использование constexpr

```cpp
// Compile-time инициализация и вычисления
constexpr array<int, 5> make_fibonacci() {
    array<int, 5> result{};
    result[0] = 1;
    result[1] = 1;
    for (size_t i = 2; i < 5; ++i) {
        result[i] = result[i-1] + result[i-2];
    }
    return result;
}

// Это выполняется во время компиляции!
constexpr auto fibonacci = make_fibonacci();
// fibonacci теперь содержит {1, 1, 2, 3, 5}

// Compile-time проверки размера
template<typename ArrayType>
constexpr bool is_small_array() {
    return ArrayType{}.size() <= 10;
}

static_assert(is_small_array<array<int, 5>>(), "Should be small");
static_assert(!is_small_array<array<int, 100>>(), "Should be large");
```

### Template метапрограммирование с array

```cpp
// Функция для создания array из parameter pack
template<typename T, T... values>
constexpr array<T, sizeof...(values)> make_array() {
    return {values...};
}

// Использование:
constexpr auto powers_of_2 = make_array<int, 1, 2, 4, 8, 16, 32>();

// Автоматический вывод размера из initializer_list (C++17)
template<typename T, typename... U>
constexpr array<T, 1 + sizeof...(U)> make_array(T&& first, U&&... rest) {
    return {std::forward<T>(first), std::forward<U>(rest)...};
}

constexpr auto numbers = make_array(1, 2, 3, 4, 5);  // array<int, 5>
```

---

## Управление памятью

### Стек vs куча: принципиальные различия

```cpp
void memory_layout_analysis() {
    // 1. array на стеке - автоматическое управление памятью
    {
        array<int, 1000> stack_array;  // 4000 байт на стеке
        // При выходе из области видимости - автоматически уничтожается
    } // Деструктор вызывается автоматически
    
    // 2. vector на куче - динамическое управление
    {
        vector<int> heap_vector(1000);  // 24 байта на стеке + 4000 байт на куче
        // Требует явного освобождения памяти в деструкторе
    } // Деструктор вызывает delete[] для освобождения кучи
    
    // 3. array в динамической памяти
    auto* dynamic_array = new array<int, 1000>;  // 4000 байт на куче
    delete dynamic_array;  // Требует явного удаления
}
```

### RAII и исключительная безопасность

```cpp
template <typename T, size_t N>
class array {
public:
    // Конструктор по умолчанию
    constexpr array() : data_{} {}  
    // Вызывает T() для каждого элемента
    
    // Конструктор из initializer_list
    array(std::initializer_list<value_type> const &items) : data_{} {
        if (items.size() > N) {
            throw std::length_error("Too many initializers for array");
        }
        
        size_t i = 0;
        for (const auto& item : items) {
            data_[i++] = item;  // Может бросить исключение T::operator=
        }
        // Оставшиеся элементы остаются в состоянии по умолчанию
    }
    
    // Деструктор = default работает корректно
    // Автоматически вызывает ~T() для каждого элемента
    ~array() = default;
    
    // Copy/move семантика
    array(const array& other) = default;      // Поэлементное копирование
    array(array&& other) noexcept = default;  // Поэлементное перемещение
    
    array& operator=(const array& other) = default;
    array& operator=(array&& other) noexcept = default;
};
```

### Exception safety анализ

```cpp
// Strong exception safety в конструкторе
array(std::initializer_list<value_type> const &items) {
    // data_ уже инициализирована через list initialization
    
    if (items.size() > N) {
        // Исключение бросается до изменения состояния объекта
        throw std::length_error("Too many initializers for array");
    }
    
    try {
        size_t i = 0;
        for (const auto& item : items) {
            data_[i] = item;  // Если бросает исключение на элементе №k
            ++i;              // то элементы [0, k-1] уже корректно присвоены
                             // а элементы [k+1, N-1] остаются в состоянии по умолчанию
        }
    } catch (...) {
        // Объект остается в частично инициализированном, но валидном состоянии
        // Деструктор корректно уничтожит все элементы
        throw;
    }
}
```

---

## Итераторы как указатели

### Оптимизация итераторов до указателей

```cpp
template <typename T, size_t N>
class array {
public:
    using iterator = T*;                    // Просто указатель!
    using const_iterator = const T*;        // Константный указатель!
    
    // Методы итераторов сводятся к арифметике указателей
    constexpr iterator begin() noexcept { 
        return data_; 
    }
    
    constexpr iterator end() noexcept { 
        return data_ + N; 
    }
    
    constexpr const_iterator begin() const noexcept { 
        return data_; 
    }
    
    constexpr const_iterator end() const noexcept { 
        return data_ + N; 
    }
    
    // C++11 cbegin/cend для явного получения const итераторов
    constexpr const_iterator cbegin() const noexcept { 
        return data_; 
    }
    
    constexpr const_iterator cend() const noexcept { 
        return data_ + N; 
    }
};
```

### Преимущества указателей как итераторов

```cpp
void iterator_performance_analysis() {
    array<int, 1000> arr;
    
    // 1. Максимальная производительность - нет накладных расходов
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        *it = 42;  // Компилируется в простую арифметику указателей
    }
    
    // 2. Совместимость с C-стилевыми функциями
    std::qsort(arr.data(), arr.size(), sizeof(int), compare_ints);
    
    // 3. Прямое взаимодействие с системными API
    write(STDOUT_FILENO, arr.data(), arr.size() * sizeof(int));
    
    // 4. Zero-cost абстракция
    static_assert(sizeof(array<int, 10>::iterator) == sizeof(int*));
}
```

### Совместимость с STL алгоритмами

```cpp
void stl_compatibility_demonstration() {
    array<int, 10> arr = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
    
    // Random Access Iterator - все алгоритмы доступны
    std::sort(arr.begin(), arr.end());                    // O(n log n)
    
    auto it = std::lower_bound(arr.begin(), arr.end(), 5); // O(log n)
    
    std::random_shuffle(arr.begin(), arr.end());           // O(n)
    
    bool is_sorted = std::is_sorted(arr.begin(), arr.end()); // O(n)
    
    // Специализированные алгоритмы используют указатели напрямую
    int* raw_ptr = arr.data();
    std::memset(raw_ptr, 0, arr.size() * sizeof(int));   // Максимально быстрое обнуление
}
```

---

## Zero-size array проблема

### Проблематика массивов нулевого размера

```cpp
template <typename T, size_t N>
class array {
private:
    // ❌ Проблемный код:
    // T data_[N];  // Если N == 0, то это не валидный C++
    
    // ✅ Решение:
    T data_[N > 0 ? N : 1];  // Всегда создаем минимум 1 элемент
    
    // Альтернативное решение с условной компиляцией:
    using storage_type = typename std::conditional<
        N == 0,
        struct { T dummy_; },  // Фиктивная структура для N=0
        T[N]                   // Обычный массив для N>0
    >::type;
    
    storage_type data_;
};
```

### Специализация для empty array

```cpp
// Полная специализация для пустого массива
template <typename T>
class array<T, 0> {
public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;
    using iterator = T*;
    using const_iterator = const T*;
    
    // Все методы размера возвращают 0
    constexpr bool empty() const noexcept { return true; }
    constexpr size_type size() const noexcept { return 0; }
    constexpr size_type max_size() const noexcept { return 0; }
    
    // Итераторы - nullptr
    constexpr iterator begin() noexcept { return nullptr; }
    constexpr iterator end() noexcept { return nullptr; }
    constexpr const_iterator begin() const noexcept { return nullptr; }
    constexpr const_iterator end() const noexcept { return nullptr; }
    
    // Доступ к элементам всегда некорректен
    [[noreturn]] reference at(size_type pos) {
        throw std::out_of_range("array<T, 0>::at");
    }
    
    [[noreturn]] const_reference at(size_type pos) const {
        throw std::out_of_range("array<T, 0>::at");
    }
    
    // operator[] может быть undefined behavior для пустого массива
    reference operator[](size_type pos) noexcept {
        // UB, но некоторые реализации возвращают *begin()
        __builtin_unreachable();  // GCC builtin
    }
    
    // data() возвращает nullptr
    constexpr T* data() noexcept { return nullptr; }
    constexpr const T* data() const noexcept { return nullptr; }
    
    // fill() не делает ничего
    void fill(const T& value) noexcept {}
    
    // swap() тоже тривиален
    void swap(array& other) noexcept {}
};
```

### Практическое использование empty array

```cpp
// Template метапрограммирование с пустыми массивами
template<bool Condition, typename T, size_t Size>
using conditional_array = array<T, Condition ? Size : 0>;

template<typename... Args>
struct variadic_array {
    using type = array<
        std::common_type_t<Args...>, 
        sizeof...(Args)
    >;
};

// Специализация для пустого parameter pack
template<>
struct variadic_array<> {
    using type = array<int, 0>;  // Произвольный тип, размер 0
};

// Compile-time условные массивы
template<size_t N>
void process_data() {
    conditional_array<(N > 0), int, N> buffer;
    
    if constexpr (N > 0) {
        // Работаем с непустым буфером
        buffer.fill(42);
    }
    // Для N=0 код компилируется, но не выполняется
}
```

---

## Оптимизации компилятора

### Анализ оптимизаций на уровне ассемблера

```cpp
// Исходный код
void test_array_performance() {
    array<int, 4> arr = {1, 2, 3, 4};
    
    for (size_t i = 0; i < arr.size(); ++i) {
        arr[i] *= 2;
    }
    
    int sum = 0;
    for (const auto& val : arr) {
        sum += val;
    }
}

// С оптимизацией -O2 компилятор может превратить это в:
void test_array_performance_optimized() {
    // Развертывание цикла (loop unrolling)
    int arr0 = 1 * 2;  // = 2
    int arr1 = 2 * 2;  // = 4  
    int arr2 = 3 * 2;  // = 6
    int arr3 = 4 * 2;  // = 8
    
    // Константное вычисление суммы
    int sum = 2 + 4 + 6 + 8;  // = 20
    
    // Или даже просто:
    // constexpr int sum = 20;
}
```

### Векторизация и SIMD оптимизации

```cpp
// Код, поддающийся векторизации
void vectorizable_operations() {
    array<float, 16> a, b, c;  // Кратно размеру SIMD регистра
    
    // Заполняем массивы
    a.fill(1.0f);
    b.fill(2.0f);
    
    // Поэлементные операции - идеально для SIMD
    for (size_t i = 0; i < 16; ++i) {
        c[i] = a[i] + b[i] * 3.14f;
    }
    
    // Компилятор может использовать:
    // - SSE/AVX инструкции (x86)
    // - NEON инструкции (ARM)
    // - Автоматическую векторизацию
}

// Пример ассемблера с AVX (упрощенно):
// vmovaps ymm0, [a]        ; Загрузить 8 float из a
// vmovaps ymm1, [b]        ; Загрузить 8 float из b  
// vmulps  ymm1, ymm1, ymm2 ; Умножить b на константу
// vaddps  ymm0, ymm0, ymm1 ; Сложить a + b*const
// vmovaps [c], ymm0        ; Сохранить результат в c
```

### Constexpr оптимизации

```cpp
// Compile-time вычисления матричных операций
template<size_t N>
constexpr array<array<int, N>, N> make_identity_matrix() {
    array<array<int, N>, N> result{};
    
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ++j) {
            result[i][j] = (i == j) ? 1 : 0;
        }
    }
    return result;
}

// Это вычисляется во время компиляции!
constexpr auto identity_3x3 = make_identity_matrix<3>();

// В ассемблере будет просто:
// .data
// identity_3x3:
//     .int 1, 0, 0
//     .int 0, 1, 0  
//     .int 0, 0, 1
```

---

## Практические примеры

### 1. Compile-time конфигурация системы

```cpp
// Конфигурация на основе array для embedded систем
template<size_t MaxSensors>
class SensorSystem {
private:
    struct SensorConfig {
        uint8_t pin;
        uint16_t threshold;
        bool enabled;
    };
    
    array<SensorConfig, MaxSensors> sensors_;
    array<uint16_t, MaxSensors> last_readings_;
    
public:
    constexpr SensorSystem() : sensors_{}, last_readings_{} {}
    
    constexpr void configure_sensor(size_t index, uint8_t pin, uint16_t threshold) {
        if (index < MaxSensors) {
            sensors_[index] = {pin, threshold, true};
        }
    }
    
    void update_readings() {
        for (size_t i = 0; i < MaxSensors; ++i) {
            if (sensors_[i].enabled) {
                last_readings_[i] = read_adc(sensors_[i].pin);
                
                if (last_readings_[i] > sensors_[i].threshold) {
                    trigger_alarm(i);
                }
            }
        }
    }
    
    constexpr size_t sensor_count() const { return MaxSensors; }
    constexpr bool has_sensor(size_t index) const {
        return index < MaxSensors && sensors_[index].enabled;
    }
};

// Compile-time конфигурация
constexpr SensorSystem<4> create_sensor_system() {
    SensorSystem<4> system;
    system.configure_sensor(0, 1, 512);  // Температурный датчик
    system.configure_sensor(1, 2, 768);  // Датчик влажности
    system.configure_sensor(2, 3, 256);  // Датчик освещенности
    // Четвертый датчик отключен
    return system;
}

constexpr auto global_sensors = create_sensor_system();
```

### 2. Lookup таблицы и математические константы

```cpp
// Compile-time генерация таблиц синусов
constexpr array<float, 360> generate_sin_table() {
    array<float, 360> result{};
    constexpr float PI = 3.14159265359f;
    
    for (size_t i = 0; i < 360; ++i) {
        float angle = i * PI / 180.0f;
        result[i] = std::sin(angle);  // Если поддерживается constexpr
    }
    
    return result;
}

// Быстрый приблизительный синус через lookup таблицу
class FastMath {
private:
    static constexpr auto sin_table = generate_sin_table();
    
public:
    static float fast_sin(float degrees) {
        // Нормализуем угол к [0, 360)
        while (degrees < 0) degrees += 360;
        while (degrees >= 360) degrees -= 360;
        
        // Интерполяция между соседними значениями
        size_t index = static_cast<size_t>(degrees);
        float frac = degrees - index;
        
        float v1 = sin_table[index];
        float v2 = sin_table[(index + 1) % 360];
        
        return v1 + frac * (v2 - v1);
    }
};

// CRC таблицы для быстрой проверки целостности
constexpr array<uint32_t, 256> generate_crc32_table() {
    array<uint32_t, 256> table{};
    
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
        table[i] = crc;
    }
    
    return table;
}

uint32_t fast_crc32(const uint8_t* data, size_t length) {
    static constexpr auto crc_table = generate_crc32_table();
    
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < length; ++i) {
        uint8_t byte = data[i];
        crc = crc_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFF;
}
```

### 3. Статический граф и алгоритмы на нем

```cpp
// Граф с фиксированным количеством узлов
template<size_t NodeCount>
class StaticGraph {
private:
    using AdjMatrix = array<array<int, NodeCount>, NodeCount>;
    AdjMatrix adj_matrix_;
    
public:
    constexpr StaticGraph() : adj_matrix_{} {
        // Инициализируем матрицу смежности
        for (size_t i = 0; i < NodeCount; ++i) {
            for (size_t j = 0; j < NodeCount; ++j) {
                adj_matrix_[i][j] = (i == j) ? 0 : -1;  // -1 = нет ребра
            }
        }
    }
    
    constexpr void add_edge(size_t from, size_t to, int weight) {
        if (from < NodeCount && to < NodeCount) {
            adj_matrix_[from][to] = weight;
        }
    }
    
    // Алгоритм Флойда-Уоршалла для поиска кратчайших путей
    array<array<int, NodeCount>, NodeCount> floyd_warshall() const {
        auto dist = adj_matrix_;
        
        // Инициализация: -1 означает бесконечность  
        for (size_t i = 0; i < NodeCount; ++i) {
            for (size_t j = 0; j < NodeCount; ++j) {
                if (i != j && dist[i][j] == -1) {
                    dist[i][j] = INT_MAX / 2;  // "Бесконечность"
                }
            }
        }
        
        // Основной алгоритм Floyd-Warshall
        for (size_t k = 0; k < NodeCount; ++k) {
            for (size_t i = 0; i < NodeCount; ++i) {
                for (size_t j = 0; j < NodeCount; ++j) {
                    if (dist[i][k] + dist[k][j] < dist[i][j]) {
                        dist[i][j] = dist[i][k] + dist[k][j];
                    }
                }
            }
        }
        
        return dist;
    }
    
    // Подсчет количества треугольников (циклов длины 3)
    constexpr size_t count_triangles() const {
        size_t count = 0;
        
        for (size_t i = 0; i < NodeCount; ++i) {
            for (size_t j = i + 1; j < NodeCount; ++j) {
                for (size_t k = j + 1; k < NodeCount; ++k) {
                    // Проверяем, есть ли ребра между всеми парами узлов
                    bool edge_ij = (adj_matrix_[i][j] != -1);
                    bool edge_jk = (adj_matrix_[j][k] != -1);  
                    bool edge_ik = (adj_matrix_[i][k] != -1);
                    
                    if (edge_ij && edge_jk && edge_ik) {
                        ++count;
                    }
                }
            }
        }
        
        return count;
    }
};

// Пример использования для моделирования сети
constexpr StaticGraph<5> create_network_topology() {
    StaticGraph<5> network;
    
    // Добавляем рёбра (связи между узлами сети)
    network.add_edge(0, 1, 10);  // Router0 -> Router1, латентность 10ms
    network.add_edge(1, 2, 15);  // Router1 -> Router2, латентность 15ms
    network.add_edge(2, 3, 20);  // Router2 -> Router3, латентность 20ms
    network.add_edge(3, 4, 25);  // Router3 -> Router4, латентность 25ms
    network.add_edge(0, 4, 100); // Router0 -> Router4, прямая связь, латентность 100ms
    
    return network;
}

constexpr auto network = create_network_topology();
```

### 4. Шифрование и криптографические операции

```cpp
// AES S-Box как compile-time array
constexpr array<uint8_t, 256> generate_aes_sbox() {
    array<uint8_t, 256> sbox{};
    
    // Упрощенная версия генерации S-Box для AES
    // (в реальности это более сложный процесс)
    for (size_t i = 0; i < 256; ++i) {
        sbox[i] = static_cast<uint8_t>(i);  // Заглушка
    }
    
    return sbox;
}

template<size_t KeyLength>
class AESCipher {
private:
    static constexpr auto s_box = generate_aes_sbox();
    array<uint8_t, KeyLength> key_;
    
public:
    constexpr AESCipher(const array<uint8_t, KeyLength>& key) : key_(key) {}
    
    // Простейший пример шифрования (не настоящий AES!)
    array<uint8_t, 16> encrypt_block(const array<uint8_t, 16>& plaintext) const {
        array<uint8_t, 16> ciphertext;
        
        for (size_t i = 0; i < 16; ++i) {
            // Применяем S-Box трансформацию
            uint8_t byte = plaintext[i];
            byte = s_box[byte];
            
            // Простое XOR с ключом (в реальном AES гораздо сложнее)
            byte ^= key_[i % KeyLength];
            
            ciphertext[i] = byte;
        }
        
        return ciphertext;
    }
    
    constexpr size_t key_length() const { return KeyLength; }
};

// Использование с compile-time ключом
constexpr array<uint8_t, 16> master_key = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

constexpr AESCipher<16> cipher(master_key);

void encrypt_data() {
    array<uint8_t, 16> message = {'H','e','l','l','o',' ','W','o','r','l','d','!',0,0,0,0};
    auto encrypted = cipher.encrypt_block(message);
    
    // encrypted содержит зашифрованные данные
}
```

---

## Заключение

Array в реализации s21 представляет собой минималистичную обертку над C-массивом, которая демонстрирует принципы zero-cost абстракции в C++:

### Ключевые архитектурные принципы:

1. **Zero Runtime Overhead** - никаких накладных расходов по сравнению с голым C-массивом
2. **Compile-time Safety** - размер проверяется на этапе компиляции
3. **STL Compatibility** - полная совместимость с алгоритмами и идиомами STL
4. **Constexpr Everything** - максимальное использование compile-time вычислений

### Технические преимущества:

- **Память на стеке** - быстрое создание/уничтожение, отличная локальность
- **Итераторы-указатели** - максимальная производительность, совместимость с C API
- **Template метапрограммирование** - мощные compile-time возможности
- **Compiler optimizations** - векторизация, развертывание циклов, константное складывание

### Ограничения и компромиссы:

- **Фиксированный размер** - нельзя изменить размер после компиляции
- **Stack overflow риск** - большие массивы могут переполнить стек
- **Template bloat** - каждый размер создает отдельную инстанциацию
- **Zero-size специализация** - требует дополнительной логики

Array идеально подходит для ситуаций, где размер данных известен на этапе компиляции и требуется максимальная производительность: embedded системы, математические вычисления, lookup таблицы, конфигурация и другие compile-time структуры данных.