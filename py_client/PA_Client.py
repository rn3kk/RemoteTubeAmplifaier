import collections
import json
import logging
import re
import socket
import sys
import threading
import time
import traceback
from sys import platform
from threading import Thread

from common.common import *


class PA_Client(Thread):
    __terminate = False
    __out_queue = collections.deque(maxlen=15)
    __mutex = threading.Lock()

    __autorisation_token = 'dfss-s24s-d4-wqe-2-ew-dswd'

    def __init__(self):
        Thread.__init__(self)
        print('PA_Client')

    def ste_terminate(self):
        self.__terminate = True

    def run(self) -> None:
        print('RadioControlClient RUN')
        while not self.__terminate:
            try:
                conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                conn.settimeout(1)
                # sett = Settings.getInstance()
                # conn.connect((sett.server_address, sett.control_port))
                conn.connect(('127.0.0.1', 6994))
                conn.setblocking(False)
                self.__set_keep_alive(conn)
                print('connected')
                self.send_to_server(Protocol.createCmd(CMD_AUTORISATION_TOKEN, self.__autorisation_token))
                while not self.__terminate:
                    while self.__out_queue.__len__() > 0:
                        self.__mutex.acquire()
                        j = self.__out_queue.popleft()
                        self.__mutex.release()
                        #print(j)
                        data = conn.send(j)
                        if not data:
                            conn.close()
                            print('disconnected1')
                            break
                    try:
                        data = conn.recv(1024)
                        # print(data)
                        if not data:
                            logging.debug('disconnected2')
                            break
                        s = re.compile(b"{.*?}")
                        m = s.search(data)
                        while m:
                            data = data.replace(m.group(), b'')
                            # print('REPLACE', data)
                            try:
                                json_cmd = json.loads(m.group())
                            except Exception as ee:
                                print('parse json error', ee, m.group())

                            self.execute_cmd(json_cmd)
                            m = s.search(data)

                    except socket.timeout:
                        continue
                    except BlockingIOError:
                        pass
                    time.sleep(0.001)
            except Exception as e:
                traceback.print_exc(file=sys.stdout)
                print
            finally:
                pass
            time.sleep(1)
        print('RadioClient run is end')

    def __set_keep_alive(self, sock, after_idle_sec=1, interval_sec=5, max_fails=6):
        if platform == 'linux':
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, after_idle_sec)
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, interval_sec)
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, max_fails)
        elif platform == 'win32':
            sock.ioctl(socket.SIO_KEEPALIVE_VALS, (1, 5000, 6000))

    def send_to_server(self, data):
        self.__mutex.acquire()
        self.__out_queue.append(data)
        self.__mutex.release()