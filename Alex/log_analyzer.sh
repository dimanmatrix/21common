#!/bin/bash

path="$1"

# Проверка на существование файла и его не пустоту
if [ ! -e "$path" ]; then
    echo "Ошибка: файл '$path' не существует."
    exit 1
fi

if [ ! -s "$path" ]; then
    echo "Ошибка: файл '$path' пуст."
    exit 1
fi

echo "$(wc -l $path | awk '{print $1}') $(cat $path | awk -F' - ' '{print $1}' | sort -u | wc -l | awk '{print $1}') $(cat $path| awk -F' - ' '{print $1 " " $3 " " $4}' | sort | awk '
    $4 != "NULL" {
        if( $1 != prev_file || $4 != prev_hash)
        {
            if (prev_file != "" || changes==0) changes++
            prev_file = $1
            prev_hash = $4
        }
    } END {print changes }
'
)"  



