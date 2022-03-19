import collections
import io
import json
import logging
import threading

from common.common import *

def is_raspberrypi():
    try:
        with io.open('/sys/firmware/devicetree/base/model', 'r') as m:
            if 'raspberry pi' in m.read().lower(): return True
    except Exception:
        pass
    return False


RASPBERRY = is_raspberrypi()

if RASPBERRY:
    import RPi.GPIO as GPIO

PWR_PIN = 17
PTT_AVALEABLE_PIN = 27
CLIENT_CONNECTED = 10
PROTECTION_PIN = 22

log = logging.getLogger('root')


class Pins(threading.Thread):
    __pwr = False
    __ptt = False
    __key = False
    __is_changed = True

    __to_client_queue = collections.deque(maxlen=15)
    __mutex = threading.Lock()

    def __init__(self):
        log.debug('Pins()')
        if RASPBERRY:
            GPIO.setwarnings(False)
            GPIO.setmode(GPIO.BCM)
            GPIO.setup(PWR_PIN, GPIO.OUT)
            GPIO.setup(PTT_AVALEABLE_PIN, GPIO.OUT)
            GPIO.setup(PROTECTION_PIN, GPIO.IN)
            GPIO.setup(CLIENT_CONNECTED, GPIO.OUT)

            GPIO.output(PWR_PIN, 0)
            GPIO.output(PTT_AVALEABLE_PIN, 0)
            GPIO.output(CLIENT_CONNECTED, 0)

    def __del__(self):
        log.debug('~Pins()')

    def __new__(cls, *args, **kwargs):
        if not hasattr(cls, 'instance'):
            cls.instance = super(Pins, cls).__new__(cls)
        return cls.instance


    def is_ptt(self):
        return self.__ptt

    def is_changed(self):
        return self.__is_changed

    def get_pins_state(self):
        self.__is_changed = False
        if self.__pwr:
            pwr = 1
        else:
            pwr = 0

        if self.__ptt:
            ptt = 1
        else:
            ptt = 0
        return pwr, ptt

    def change_pwr(self):
        self.__pwr = not self.__pwr
        if RASPBERRY:
            GPIO.output(PWR_PIN, self.__pwr)
        log.debug('power = ' + str(self.__pwr))
        self.__is_changed = True

        p = {}
        p[COMMAND] = FROM_PA_PIN_PWR_STATE
        p[VALUE] = int(self.__pwr)
        self.__add_data(json.dumps(p).encode())


    def client_connected(self):
        if RASPBERRY:
            GPIO.output(CLIENT_CONNECTED, True)

    def client_disconnected(self):
        if RASPBERRY:
            GPIO.output(CLIENT_CONNECTED, False)
            self.__ptt = False


    def json_cmd(self, cmd):
        c = cmd[COMMAND]
        if c == CMD_CHANGE_PWR_PIN:
            self.change_pwr()


    def __add_data(self, data):
        self.__mutex.acquire()
        self.__to_client_queue.append(data)
        self.__mutex.release()

    def get_data(self):
        if not self.__to_client_queue.__len__():
            return None

        self.__mutex.acquire()
        d = self.__to_client_queue.popleft()
        self.__mutex.release()
        return d

    def send_status(self):
        data = {}
        data[COMMAND] = FROM_PA_PIN_PWR_STATE
        data[VALUE] = int(self.__ptt)
        self.__add_data(json.dumps(data).encode())

        data = {}
        data[COMMAND] = FROM_RADIO_PINS_PWR_STATE
        data[VALUE] = int(self.__pwr)
        self.__add_data(json.dumps(data).encode())
