import collections
import logging
import threading
import time
from threading import Thread

from common.common import *

if RASPBERRY:
    import RPi.GPIO as GPIO

PWR_PIN = 4
PTT_PIN = 17
CLIENT_CONNECTED = 27
PTT_REQUEST_PIN = 22

PROTECTION_PIN = 10  # этот пин идицируется если сработала защита
PROTECTION_PIN_2 = 9  # этот пин говорит что птт не включится, т.к. например крутится мотор БМЗ
PROTECTION_RESET_PIN = 11

# fo Rp pi1 b
RELAY1 = 21
RELAY2 = 20
RELAY3 = 16
RELAY4 = 12
RELAY5 = 25
RELAY6 = 24
RELAY7 = 23
RELAY8 = 18

log = logging.getLogger('root')


class Pins(Thread):
    __bandpath = False #обход
    __pwr = False
    __ptt_out = False  # cmd to TX
    __ptt_req = False  # request to TX
    __protection = False
    __protection2 = False
    __is_changed = True
    __relay_num = 0
    __relay = {}

    __to_client_queue = collections.deque(maxlen=15)
    __mutex = threading.Lock()

    __terminate = False

    def __init__(self):
        # log.debug('Pins()')
        Thread.__init__(self)

        self.__relay[1] = RELAY1
        self.__relay[2] = RELAY2
        self.__relay[3] = RELAY3
        self.__relay[4] = RELAY4
        self.__relay[5] = RELAY5
        self.__relay[6] = RELAY6
        self.__relay[7] = RELAY7
        self.__relay[8] = RELAY8

        if RASPBERRY:
            GPIO.setwarnings(False)
            GPIO.setmode(GPIO.BCM)
            GPIO.setup(PWR_PIN, GPIO.OUT)
            GPIO.setup(PTT_PIN, GPIO.OUT)
            GPIO.setup(PROTECTION_PIN, GPIO.IN)
            GPIO.setup(PROTECTION_PIN_2, GPIO.IN)
            GPIO.setup(CLIENT_CONNECTED, GPIO.OUT)
            GPIO.setup(PROTECTION_RESET_PIN, GPIO.OUT)
            GPIO.setup(PTT_REQUEST_PIN, GPIO.IN)
            for i in range(1, 8):
                GPIO.setup(self.__relay[i], GPIO.OUT)

            GPIO.output(PWR_PIN, 0)
            GPIO.output(PTT_PIN, 0)
            GPIO.output(CLIENT_CONNECTED, 0)
            GPIO.output(PROTECTION_RESET_PIN, 0)
            self.__reset_relay_pins()

    def __del__(self):
        print('~Pins()')

    def __new__(cls, *args, **kwargs):
        if not hasattr(cls, 'instance'):
            cls.instance = super(Pins, cls).__new__(cls)
        return cls.instance

    def run(self) -> None:
        if not RASPBERRY:
            return

        while not self.__terminate:
            time.sleep(0.001)

            prot1 = GPIO.input(PROTECTION_PIN)
            if prot1 != self.__protection:
                self.__add_data(Protocol.createCmd(FROM_PA_PIN_PROTECTION_STATE, prot1))
                self.__protection = prot1

            prot2 = GPIO.input(PROTECTION_PIN_2)
            if prot2 != self.__protection:
                self.__add_data(Protocol.createCmd(FROM_PA_PIN_2_PROTECTION_STATE, prot2))
                self.__protection = prot2

            if not self.__bandpath:
                ptt_req = GPIO.input(PTT_REQUEST_PIN)
                if ptt_req != self.__ptt_req:
                    self.__ptt_req = ptt_req
                    self.change_ptt(ptt_req)
            else:
                if self.__ptt_out:
                    self.change_ptt(False)

    def change_pwr(self):
        self.__pwr = not self.__pwr
        if RASPBERRY:
            GPIO.output(PWR_PIN, self.__pwr)
        log.debug('power = ' + str(self.__pwr))

        d = Protocol.createCmd(FROM_PA_PIN_PWR_STATE, int(self.__pwr))
        self.__add_data(d)

    def change_bandpath(self):
        self.__bandpath = not self.__bandpath
        log.debug(f'Bandpass is {self.__bandpath}')
        d = Protocol.createCmd(FROM_PA_BANDPASS, int(self.__bandpath))
        self.__add_data(d)

    def change_ptt(self, state):
        if self.__protection or self.__protection2:
            if not self.__pwr:
                return
            GPIO.output(PTT_PIN, 0)
            self.__pwr = False
            return
        if self.__ptt_out == state:
            return
        self.__ptt_out = state
        if RASPBERRY:
            GPIO.output(PTT_PIN, self.__ptt_out)

    def reset_protection(self):
        if RASPBERRY:
            log.debug('Start reset protection PIN')
            GPIO.output(PROTECTION_RESET_PIN, True)
            time.sleep(0.2)
            GPIO.output(PROTECTION_RESET_PIN, False)
            log.debug('End reset protection PIN')

    def client_connected(self):
        self.__send_status()
        if RASPBERRY:
            GPIO.output(CLIENT_CONNECTED, True)

    def client_disconnected(self):
        if RASPBERRY:
            GPIO.output(CLIENT_CONNECTED, False)

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
        d += Protocol.createCmd(FROM_PA_PTT_STATE, int(self.__ptt_out))
        d += Protocol.createCmd(FROM_PA_PIN_PROTECTION_STATE, int(self.__protection))
        d += Protocol.createCmd(FROM_PA_PIN_2_PROTECTION_STATE, int(self.__protection2))
        d += Protocol.createCmd(FROM_PA_RELAY_NUM, self.__relay_num)
        d += Protocol.createCmd(FROM_PA_BANDPASS, int(self.__bandpath))
        self.__add_data(d)

    def set_terminate(self):
        print('PA_TCP_Server set_terminate')
        self.__terminate = True

    def set_relay_number(self, num):
        if num < 0 or num > 8:
            num = 0
        self.__reset_relay_pins()
        if num > 0 and RASPBERRY:
            GPIO.output(self.__relay[num], 1)
        self.__relay_num = num
        d = Protocol.createCmd(FROM_PA_RELAY_NUM, num)
        self.__add_data(d)

    def __reset_relay_pins(self):
        if not RASPBERRY:
            return
        for i in range(1, 8):
            try:
                GPIO.output(self.__relay[i], 0)
            except Exception as e:
                log.exception(e)
                print('cant reset pin', self.__relay[i])
