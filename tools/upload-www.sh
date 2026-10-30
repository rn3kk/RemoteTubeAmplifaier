#!/bin/bash
# Собирает образ LittleFS из esp32-backend/data и заливает его в раздел файлов ESP32.
# Использование: tools/upload-www.sh [порт]   (по умолчанию /dev/ttyACM0)
# Перед запуском закройте монитор порта.
set -e

PORT="${1:-/dev/ttyACM0}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DATA="$ROOT/esp32-backend/data"
TOOLS="$HOME/.arduino15/packages/esp32/tools"

# Раздел spiffs из таблицы разделов default.csv ядра ESP32 (её использует прошивка)
OFFSET=0x290000
SIZE=$((0x160000))

MKLITTLEFS="$(ls "$TOOLS"/mklittlefs/*/mklittlefs 2>/dev/null | sort -V | tail -n 1)"
ESPTOOL="$(ls "$TOOLS"/esptool_py/*/esptool 2>/dev/null | sort -V | tail -n 1)"
if [ -z "$MKLITTLEFS" ] || [ -z "$ESPTOOL" ]; then
    echo "Не найдены mklittlefs или esptool в $TOOLS: установите ядро esp32:esp32" >&2
    exit 1
fi

IMAGE="$(mktemp --suffix=.bin)"
trap 'rm -f "$IMAGE"' EXIT

"$MKLITTLEFS" -c "$DATA" -p 256 -b 4096 -s "$SIZE" "$IMAGE"
"$ESPTOOL" --chip esp32s3 --port "$PORT" write-flash "$OFFSET" "$IMAGE"
