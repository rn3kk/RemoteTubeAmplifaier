#!/usr/bin/env python3
"""Проверка сервоприводов Feetech STS3215 с компьютера через USB адаптера шины.

Адаптер Waveshare Bus Servo Adapter (A): перемычки в положение B, USB-C к компьютеру,
питание приводов 12 В. ESP32 в это время от шины отключена.

Использование: tools/sts-servo.py [-p порт] команда [аргументы]
  scan                  найти приводы (ID 0..253) и прочитать их положение
  regs ID               режим, ограничения угла, удержание, цель, положение, напряжение, температура
  goto ID ЦЕЛЬ          задать цель положения (в многооборотном режиме может быть < 0 или > 4095)
                        и показать, как привод едет
  powercut ID [ОБОРОТЫ] медленное вращение на несколько оборотов: выключите и включите 12 В
                        во время вращения - скрипт покажет угол до и после пропадания питания
  multiturn ID          включить многооборотный режим (режим 0, ограничения угла 0) только в ОЗУ привода
  resetturns ID         сбросить счёт оборотов к углу: режим 3 и обратно 0 только в ОЗУ привода
Записи в регистры настроек идут при защите EEPROM (регистр 55 = 1): действуют до выключения
питания привода, в его память не попадают.
Порт по умолчанию - первый /dev/serial/by-id/usb-1a86* (CH343 на адаптере).
Требуется pyserial.
"""
import glob
import sys
import time

import serial

BAUD = 1000000
REG_MIN_ANGLE = 9
REG_MAX_ANGLE = 11
REG_MODE = 33
REG_TORQUE = 40
REG_LOCK = 55
REG_ACC = 41
REG_GOAL = 42
REG_POSITION = 56
REG_VOLTAGE = 62
REG_TEMPERATURE = 63
REG_MOVING = 66
SPEED = 1500
ACC = 50


class Bus:
    def __init__(self, port):
        self.port = serial.Serial(port, BAUD, timeout=0.03)

    def transfer(self, servo_id, inst, params, reply_len):
        body = [servo_id, len(params) + 2, inst] + params
        self.port.reset_input_buffer()
        self.port.write(bytes([0xFF, 0xFF] + body + [(~sum(body)) & 0xFF]))
        self.port.flush()
        return self.port.read(reply_len)

    def ping(self, servo_id):
        return len(self.transfer(servo_id, 0x01, [], 6)) == 6

    def read(self, servo_id, reg, count):
        reply = self.transfer(servo_id, 0x02, [reg, count], 6 + count)
        if len(reply) != 6 + count:
            return None
        return list(reply[5:5 + count])

    def read_byte(self, servo_id, reg):
        data = self.read(servo_id, reg, 1)
        return None if data is None else data[0]

    def read_word(self, servo_id, reg):
        data = self.read(servo_id, reg, 2)
        return None if data is None else data[0] | (data[1] << 8)

    def write(self, servo_id, reg, data):
        self.transfer(servo_id, 0x03, [reg] + data, 6)

    def set_goal(self, servo_id, goal, speed=SPEED):
        raw = abs(goal) | (0x8000 if goal < 0 else 0)
        self.write(servo_id, REG_ACC, [ACC, raw & 0xFF, raw >> 8, 0, 0, speed & 0xFF, speed >> 8])


def signed(raw):
    if raw is None:
        return None
    return -(raw & 0x7FFF) if raw & 0x8000 else raw


def cmd_scan(bus):
    found = 0
    for servo_id in range(254):
        if bus.ping(servo_id):
            found += 1
            print('ID %d: положение %s' % (servo_id, bus.read_word(servo_id, REG_POSITION)))
    print('найдено приводов: %d' % found)


def cmd_regs(bus, servo_id):
    if not bus.ping(servo_id):
        print('ID %d не отвечает' % servo_id)
        return
    voltage = bus.read_byte(servo_id, REG_VOLTAGE)
    print('режим (33):           %s' % bus.read_byte(servo_id, REG_MODE))
    print('ограничения угла:     %s..%s (оба 0 - многооборотный режим)' %
          (bus.read_word(servo_id, REG_MIN_ANGLE), bus.read_word(servo_id, REG_MAX_ANGLE)))
    print('удержание (40):       %s' % bus.read_byte(servo_id, REG_TORQUE))
    print('цель (42):            %s' % signed(bus.read_word(servo_id, REG_GOAL)))
    print('положение (56):       %s' % bus.read_word(servo_id, REG_POSITION))
    print('напряжение:           %s В' % (None if voltage is None else voltage / 10))
    print('температура:          %s °C' % bus.read_byte(servo_id, REG_TEMPERATURE))


def cmd_goto(bus, servo_id, goal):
    print('положение до: %s, цель %d' % (bus.read_word(servo_id, REG_POSITION), goal))
    bus.set_goal(servo_id, goal)
    start = time.time()
    time.sleep(0.1)
    while time.time() - start < 10 and bus.read_byte(servo_id, REG_MOVING) == 1:
        time.sleep(0.03)
    print('встал за %.2f с, положение %s' % (time.time() - start, bus.read_word(servo_id, REG_POSITION)))


def cmd_powercut(bus, servo_id, turns):
    start_pos = bus.read_word(servo_id, REG_POSITION)
    if start_pos is None:
        print('ID %d не отвечает' % servo_id)
        return
    goal = start_pos + turns * 4096
    bus.set_goal(servo_id, goal, 700)
    print('вращение: положение %d, цель %d. Выключите и включите 12 В во время вращения' % (start_pos, goal))
    start = time.time()
    last = None
    lost = None
    while time.time() - start < 180:
        pos = bus.read_word(servo_id, REG_POSITION)
        if pos is None:
            if lost is None:
                lost = time.time()
                print('%.2f с: связь пропала, последний угол %s' % (lost - start, last))
        elif lost is not None:
            time.sleep(0.5)
            print('%.2f с: связь вернулась через %.1f с: угол %s, цель %s, удержание %s' %
                  (time.time() - start, time.time() - lost, bus.read_word(servo_id, REG_POSITION),
                   signed(bus.read_word(servo_id, REG_GOAL)), bus.read_byte(servo_id, REG_TORQUE)))
            return
        else:
            last = pos
        time.sleep(0.02)
    print('пропадания питания не было, угол %s' % bus.read_word(servo_id, REG_POSITION))


def cmd_multiturn(bus, servo_id):
    bus.write(servo_id, REG_LOCK, [1])
    bus.write(servo_id, REG_MODE, [0])
    bus.write(servo_id, REG_MIN_ANGLE, [0, 0])
    bus.write(servo_id, REG_MAX_ANGLE, [0, 0])
    cmd_regs(bus, servo_id)


def cmd_resetturns(bus, servo_id):
    print('цель до сброса: %s' % signed(bus.read_word(servo_id, REG_GOAL)))
    bus.write(servo_id, REG_LOCK, [1])
    bus.write(servo_id, REG_MODE, [3])
    bus.write(servo_id, REG_MODE, [0])
    angle = bus.read_word(servo_id, REG_POSITION)
    if angle is not None:
        bus.set_goal(servo_id, angle)  # сброс снимает удержание - возвращаем
    print('счёт сброшен к углу %s, цель %s' % (angle, signed(bus.read_word(servo_id, REG_GOAL))))


def main():
    args = sys.argv[1:]
    port = None
    if len(args) >= 2 and args[0] == '-p':
        port = args[1]
        args = args[2:]
    if not args:
        print(__doc__)
        return 1
    if port is None:
        ports = glob.glob('/dev/serial/by-id/usb-1a86*')
        if not ports:
            print('адаптер не найден: перемычки в B, USB-C к компьютеру, или укажите порт через -p')
            return 1
        port = ports[0]
    bus = Bus(port)
    command = args[0]
    if command == 'scan':
        cmd_scan(bus)
    elif command == 'regs' and len(args) == 2:
        cmd_regs(bus, int(args[1]))
    elif command == 'goto' and len(args) == 3:
        cmd_goto(bus, int(args[1]), int(args[2]))
    elif command == 'powercut' and len(args) in (2, 3):
        cmd_powercut(bus, int(args[1]), int(args[2]) if len(args) == 3 else 6)
    elif command == 'multiturn' and len(args) == 2:
        cmd_multiturn(bus, int(args[1]))
    elif command == 'resetturns' and len(args) == 2:
        cmd_resetturns(bus, int(args[1]))
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
