import collections
import json
import logging
import re
import socket
import threading
import time
from threading import Thread
from sys import platform

from common.common import Protocol, FROM_PA_TRX_FOUND, FROM_PA_TRX_FREQ
from py_server import states
from py_server.settings import Settings
log = logging.getLogger('root')

DIRECTION = "d"
COMMAND = "c"
VALUE = "v"
DATA = "data"

RADIO_EVENT_FREQ_CHANGED = 0
RADIO_EVENT_POWER_STATE = 1

TO_EXTERNAL_EXCHANGE = 6

class TRX_State_TCP_Client(Thread):
    __instance = None
    __terminate = False

    __to_client_queue = collections.deque(maxlen=15)
    __mutex = threading.Lock()

    __trx_found = False
    __trx_freq = 0

    def __init__(self):
        Thread.__init__(self)
        self.__mech1 = None
        self.__mech2 = None

    def __del__(self):
        pass

    @classmethod
    def getInstance(cls):
        if not cls.__instance:
            cls.__instance = TRX_State_TCP_Client()
            cls.__instance.start()
        return cls.__instance

    @classmethod
    def resetInstance(cls):
        if cls.__instance:
            cls.__instance.set_terminate()
            cls.__instance.join()
            cls.__instance.__del__()

    def set_terminate(self):
        self.__terminate = True

    def run(self) -> None:
        while not self.__terminate:
            try:
                conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                conn.settimeout(1)
                sett = Settings.getInstance()
                conn.connect(sett.trx_addr)
                self.__set_keep_alive(conn)
                conn.settimeout(1)
                while not self.__terminate:
                    try:
                        data = conn.recv(1024)
                        if not data:
                            self.__disconnected()
                            break
                        print(data)
                        s = re.compile(b"{.*?}")
                        m = s.search(data)
                        while m:
                            data = data.replace(m.group(), b'')
                            try:
                                json_cmd = json.loads(m.group())
                                dir = json_cmd[DIRECTION]
                                cmd = json_cmd[COMMAND]
                                value = str(json_cmd[VALUE])
                                if dir == TO_EXTERNAL_EXCHANGE:
                                    if cmd == RADIO_EVENT_POWER_STATE:
                                        if value.lower() == 'true':
                                            self.__trx_found = True
                                            states.radio_found = True
                                        else:
                                            self.__trx_found = False
                                            states.radio_found = False
                                        self.__add_data_to_client(
                                            Protocol.createCmd(FROM_PA_TRX_FOUND, self.__trx_found))
                                    elif cmd == RADIO_EVENT_FREQ_CHANGED:
                                        self.__trx_freq = int(value)
                                        states.radio_freq = int(value)
                                        self.__trx_freq_changed()
                                        self.__add_data_to_client(
                                            Protocol.createCmd(FROM_PA_TRX_FREQ, self.__trx_freq))
                            except Exception as ee:
                                print('parse json error', ee, m.group())
                            m = s.search(data)
                    except socket.timeout:
                        continue
                    except BlockingIOError:
                        pass
                    time.sleep(0.1)
            except Exception as e:
                self.__disconnected()
                log.exception(e)
            time.sleep(1)

    def __set_keep_alive(self, sock, after_idle_sec=1, interval_sec=5, max_fails=6):
        if platform == 'linux':
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, after_idle_sec)
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, interval_sec)
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, max_fails)
        elif platform == 'win32':
            sock.ioctl(socket.SIO_KEEPALIVE_VALS, (1, 5000, 6000))

    def __disconnected(self):
        log.debug('Disconnected from TRX server')
        self.__radio_state = False
        self.__radio_freq = 0

    def __connected(self):
        log.debug('Connected to TRX server')

    def __add_data_to_client(self, data):
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

    def send_current_state(self):
        d = Protocol.createCmd(FROM_PA_TRX_FOUND, self.__trx_found)
        d += Protocol.createCmd(FROM_PA_TRX_FREQ, self.__trx_freq)
        self.__add_data_to_client(d)

    def add_mechaduino(self, mech1, mech2=None):
        self.__mech1 = mech1
        self.__mech2 = mech2

    def __trx_freq_changed(self):
        if self.__trx_found:
            if self.__mech1:
                self.__mech1.changed_frequency(self.__trx_freq)
