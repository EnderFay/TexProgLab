#!/bin/bash

echo "=== Отладка сетевого трафика ==="
echo "Порт: 8080"
echo "Для остановки нажмите Ctrl+C"
echo ""

# Сохраняем вывод в файл
OUTPUT_FILE="network_debug_$(date +%Y%m%d_%H%M%S).log"

echo "Захват начинается через 3 секунды..."
sleep 3

# Захватываем трафик
sudo tcpdump -i lo port 8080 -A 2>/dev/null | tee "$OUTPUT_FILE"
