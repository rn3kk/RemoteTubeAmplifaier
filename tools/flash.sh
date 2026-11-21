#!/bin/bash
# Сборка прошивки контроллера и заливка её и файлов веб-страницы в ESP32.
#
# Использование: tools/flash.sh [-p порт] [команда]
#   all       собрать, залить прошивку и файлы веб-страницы (по умолчанию)
#   build     только собрать прошивку
#   firmware  собрать и залить прошивку
#   www       залить только файлы веб-страницы (esp32-backend/data)
#   monitor   открыть монитор порта (выход - Ctrl+C)
# Порт по умолчанию /dev/ttyACM0. Перед заливкой закройте монитор порта.
set -e

#PORT=/dev/ttyACM0
#PORT=/dev/cu.usbmodem1201
PORT=/dev/cu.usbmodem1101
if [ "$1" = "-p" ]; then
    PORT="$2"
    shift 2
fi
COMMAND="${1:-all}"

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SKETCH="$ROOT/esp32-backend"
FQBN="esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc"

ARDUINO_CLI="$(command -v arduino-cli || true)"
if [ -z "$ARDUINO_CLI" ] && [ -x "$HOME/.local/bin/arduino-cli" ]; then
    ARDUINO_CLI="$HOME/.local/bin/arduino-cli"
fi
if [ -z "$ARDUINO_CLI" ]; then
    echo "arduino-cli не найден. Установка:" >&2
    echo "  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh" >&2
    exit 1
fi

check_secrets() {
    if [ ! -f "$SKETCH/secrets.h" ]; then
        echo "Нет файла $SKETCH/secrets.h с настройками Wi-Fi. Создайте его:" >&2
        echo '  #pragma once' >&2
        echo '  const char *WIFI_SSID = "имя сети";' >&2
        echo '  const char *WIFI_PASS = "пароль";' >&2
        exit 1
    fi
}

# Порт должен существовать и не быть занят монитором
check_port() {
    for i in $(seq 1 20); do
        [ -e "$PORT" ] && break
        sleep 0.5
    done
    if [ ! -e "$PORT" ]; then
        echo "Порт $PORT не найден: подключите плату или укажите порт через -p" >&2
        exit 1
    fi
#    if command -v fuser >/dev/null && fuser "$PORT" >/dev/null 2>&1; then
#        echo "Порт $PORT занят (открыт монитор порта?): закройте его и повторите" >&2
#        exit 1
#    fi
}

build() {
    if grep -Eq '^#define[[:space:]]+USE_ETHERNET[[:space:]]+0([[:space:]]|$)' "$SKETCH/esp32-backend.ino"; then
        check_secrets
    fi
    echo "=== Сборка прошивки"
    local cpp_flags websocket_flags=""
    cpp_flags="$("$ARDUINO_CLI" compile --fqbn "$FQBN" --show-properties "$SKETCH" | sed -n 's/^compiler.cpp.flags=//p')"
    if [ -z "$cpp_flags" ]; then
        echo "Не удалось определить флаги компилятора C++ для $FQBN" >&2
        exit 1
    fi
    if grep -Eq '^#define[[:space:]]+USE_ETHERNET[[:space:]]+1([[:space:]]|$)' "$SKETCH/esp32-backend.ino"; then
        # TCI использует ws://. Ethernet-вариант библиотеки не включает Wi-Fi и TLS.
        # WebSockets 2.7.2 использует старые имена клиентов; в ядре 3.x это NetworkClient/Server.
        websocket_flags='-DWEBSOCKETS_NETWORK_TYPE=NETWORK_ESP32_ETH -DWiFiClient=NetworkClient -DWiFiServer=NetworkServer'
    fi
    # Стандарт идёт после флагов ядра ESP32, иначе ядро может переопределить его.
    "$ARDUINO_CLI" compile --fqbn "$FQBN" --build-property "compiler.cpp.flags=$cpp_flags -std=gnu++17 $websocket_flags" "$SKETCH"
}

upload_firmware() {
    check_port
    echo "=== Заливка прошивки в $PORT"
    "$ARDUINO_CLI" upload -p "$PORT" --fqbn "$FQBN" "$SKETCH"
}

upload_www() {
    check_port
    echo "=== Заливка файлов веб-страницы в $PORT"
    "$ROOT/tools/upload-www.sh" "$PORT"
}

case "$COMMAND" in
    all)
        build
        upload_firmware
        sleep 2  # после заливки плата перезагружается и порт появляется заново
        upload_www
        ;;
    build)
        build
        ;;
    firmware)
        build
        upload_firmware
        ;;
    www)
        upload_www
        ;;
    monitor)
        "$ARDUINO_CLI" monitor -p "$PORT" -c baudrate=115200
        ;;
    *)
        echo "Неизвестная команда: $COMMAND (all, build, firmware, www, monitor)" >&2
        exit 1
        ;;
esac
echo "=== Готово"
