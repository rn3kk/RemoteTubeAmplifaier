import collections
import io
import logging
import threading
import time
from threading import Thread

from common.common import *


if RASPBERRY:
    import RPi.GPIO as GPIO

PWR_PIN = 17
PTT_PIN = 27
CLIENT_CONNECTED = 10
PROTECTION_PIN = 22
PTT_REQUEST_PIN = 23

log = logging.getLogger('root')


class Pins(Thread):
    __pwr = False
    __ptt_out = False #cmd to TX
    __ptt_req = False #request to TX
    __protection = False
    __is_changed = True

    __to_client_queue = collections.deque(maxlen=15)
    __mutex = threading.Lock()

    __terminate = False

    def __init__(self):
        log.debug('Pins()')
        Thread.__init__(self)
        if RASPBERRY:
            GPIO.setwarnings(False)
            GPIO.setmode(GPIO.BCM)
            GPIO.setup(PWR_PIN, GPIO.OUT)
            GPIO.setup(PTT_PIN, GPIO.OUT)
            GPIO.setup(PROTECTION_PIN, GPIO.IN)
            GPIO.setup(CLIENT_CONNECTED, GPIO.OUT)

            GPIO.output(PWR_PIN, 0)
            GPIO.output(PTT_PIN, 0)
            GPIO.output(CLIENT_CONNECTED, 0)

    def __del__(self):
        log.debug('~Pins()')

    def __new__(cls, *args, **kwargs):
        if not hasattr(cls, 'instance'):
            cls.instance = super(Pins, cls).__new__(cls)
        return cls.instance

    def run(self) -> None:
        if not RASPBERRY:
            return
        while not self.__terminate:
            time.sleep(0.001)
            if GPIO.input(PROTECTION_PIN) == 1:
                log.warning('PROTECTION!')
                self.__change_ptt(False)
                if self.__protection == False:
                    self.__protection = True
                    d = Protocol.createCmd(FROM_PA_PIN_PROTECTION_STATE, 1)
                    self.__add_data(d)
                continue
            else:
                if self.__protection == True:
                    self.__protection = False
                    d = Protocol.createCmd(FROM_PA_PIN_PROTECTION_STATE, 1)
                    self.__add_data(d)

            if GPIO.input(PTT_REQUEST_PIN) == 1:
                ptt_req = GPIO.input(PTT_REQUEST_PIN)
                if ptt_req and not self.__protection:
                    self.__change_ptt(True)
                else:
                    self.__change_ptt(False)
            else:
                self.__change_ptt(False)

    def __change_pwr(self):
        self.__pwr = not self.__pwr
        if RASPBERRY:
            GPIO.output(PWR_PIN, self.__pwr)
        log.debug('power = ' + str(self.__pwr))

        d = Protocol.createCmd(FROM_PA_PIN_PWR_STATE, int(self.__pwr))
        self.__add_data(d)

    def __change_ptt(self, state):
        if self.__ptt_out == state:
            return
        self.__ptt_out = state
        if RASPBERRY:
            GPIO.output(PTT_PIN, self.__pwr)

    def __client_connected(self):
        if RASPBERRY:
            GPIO.output(CLIENT_CONNECTED, True)

    def __client_disconnected(self):
        if RASPBERRY:
            GPIO.output(CLIENT_CONNECTED, False)
            self.__ptt_out = False

    def json_cmd(self, cmd):
        c = cmd[COMMAND]
        if c == CMD_CHANGE_PWR_PIN:
            self.__change_pwr()

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

    def __send_status(self):
        d = Protocol.createCmd(FROM_PA_PIN_PWR_STATE, int(self.__pwr))
        d += Protocol.createCmd(FROM_PA_PIN_PWR_STATE, int(self.__ptt_out))
        d += Protocol.createCmd(FROM_PA_PIN_PROTECTION_STATE, int(self.__protection))
        self.__add_data(d)

