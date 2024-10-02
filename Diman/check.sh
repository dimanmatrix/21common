

check_log_format() {
    local line="$1"
    
    # Регулярное выражение для проверки формата
    local regex='^([^ ]+) - ([0-9]+) - ([0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}) - (NULL|[^ ]+) - ([a-z0-9]+)$'
    
    if [[ $line =~ $regex ]]; then
        echo "Формат корректен: $line"
        
        # Дополнительные проверки
        local file="${BASH_REMATCH[1]}"
        local number="${BASH_REMATCH[2]}"
        local date="${BASH_REMATCH[3]}"
        local value="${BASH_REMATCH[4]}"
        local hash="${BASH_REMATCH[5]}"
        
        # Проверка существования файла
        if [[ ! -f "$file" ]]; then
            echo "Предупреждение: Файл $file не существует"
        fi
        
        # Проверка корректности даты
        if ! date -d "$date" &>/dev/null; then
            echo "Ошибка: Некорректная дата $date"
        fi
        
        # Проверка, что hash - это sha256 (64 символа)
        if [[ ${#hash} -ne 64 ]]; then
            echo "Ошибка: Hash не похож на sha256 (должен быть 64 символа)"
            
        fi
    else
        echo "Ошибка: Некорректный формат строки: $line"
        exit 1
    fi
}