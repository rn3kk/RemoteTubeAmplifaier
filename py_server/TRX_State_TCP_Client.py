import json
import logging
import re
import socket
import time
from threading import Thread
from sys import platform

from py_server.settings import Settings
log = logging.getLogger('root')

DIRECTION = "d"
COMMAND = "c"
VALUE = "v"
DATA = "data"

RADIO_EVENT_FREQ_CHANGED = 0
RADIO_EVENT_POWER_STATE =1

TO_EXTERNAL_EXCHANGE = 6

class TRX_State_TCP_Client(Thread):
    __instance = None
    __terminate = False
    __radio_state = False
    __radio_freq = 0

    def __init__(self):
        Thread.__init__(self)

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
                                            self.__radio_state = True
                                        else:
                                            self.__radio_state = False
                                        print('radio state', self.__radio_state)
                                    elif cmd == RADIO_EVENT_FREQ_CHANGED:
                                        self.__radio_freq = value
                                        print('radio freq', self.__radio_freq)

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