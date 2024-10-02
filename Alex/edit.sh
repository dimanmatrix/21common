#!/bin/bash

if [ "$#" -ne 3 ]; then
    echo "Использование: $0 путь_файла строка_для_замены замена"
    exit 1
fi

FILE_PATH="$1"
SEARCH_STRING="$2"
REPLACEMENT_STRING="$3"

# Проверка существования файла и его ненулевой размер
if [ ! -e "$FILE_PATH" ]; then
    echo "Файл не существует: $FILE_PATH"
    exit 1
fi

if [ ! -s "$FILE_PATH" ]; then
    echo "Файл пустой: $FILE_PATH"
    exit 1
fi

# Проверка на права доступа к файлу
if [ ! -r "$FILE_PATH" ]; then
    echo "Нет прав на чтение файла: $FILE_PATH"
    exit 1
fi

if [ ! -w "$FILE_PATH" ]; then
    echo "Нет прав на запись в файл: $FILE_PATH"
    exit 1
fi

# Проверка на наличие подстроки для замены в файле
if ! grep -q "$SEARCH_STRING" "$FILE_PATH"; then
    echo "Подстрока для замены не найдена в файле: $FILE_PATH"
    exit 1
fi

# Выполнение замены с обработкой ошибок
if ! sed -i '' "s/$SEARCH_STRING/$REPLACEMENT_STRING/g" "$FILE_PATH"; then
    echo "Ошибка при выполнении команды sed"
    exit 1
fi

# Логгирование изменений
echo "$FILE_PATH - $(stat -f %z "$FILE_PATH") - $(date +"20%y-%m-%d %H:%M") - $(shasum -a 256 "$FILE_PATH" | awk '{print $1}') - sha256" >> src/files.log