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
    __mech1_angle = 0
    __mech1_manual_mode = 0
    __pa_pin_pwr = 0
    __pa_pin_ptt = 0
    __protection_pin = 0
    __relay_num = 0
    __edit_mode = 0
    __autorisation_token = 'dfss-s24s-d4-wqe-2-ew-dswd'

    __changed = False
    __changed_mutex = threading.Lock()


    def __init__(self):
        Thread.__init__(self)
        print('PA_Client')

    def set_terminate(self):
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
                        # print('Received', data)
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

    def set_mech1_angle(self, angle):
        d = Protocol.createCmd(CMD_CHANGE_ANGLE_MECH1, angle)
        self.send_to_server(d)

    def set_mech2_angle(self, angle):
        d = Protocol.createCmd(CMD_CHANGE_ANGLE_MECH2, angle)
        self.send_to_server(d)

    def mech1_manual(self):
        d = Protocol.createCmd(CMD_CHANGE_MANUAL_MECH1, 0)
        self.send_to_server(d)

    def set_power(self):
        d = Protocol.createCmd(CMD_CHANGE_PWR, 0)
        self.send_to_server(d)

    def set_relay(self, relay):
        d = Protocol.createCmd(CMD_CHANGE_RELAY, relay)
        self.send_to_server(d)

    def reset_protection(self):
        d = Protocol.createCmd(CMD_RESET_PROTECTION, 0)
        self.send_to_server(d)

    def change_pwr(self):
        d = Protocol.createCmd(CMD_CHANGE_PWR, 0)
        self.send_to_server(d)

    def change_edit_mode(self):
        d = Protocol.createCmd(CMD_EDIT_MODE, 0)
        self.send_to_server(d)

    def execute_cmd(self, data):
        cmd = data[COMMAND]
        value = data[VALUE]
        if cmd == FROM_PA_ANGLE_MECH1:
            self.__mech1_angle = int(value)
        elif cmd == FROM_PA_MECH1_MANUAL_MODE:
            self.__mech1_manual_mode = int(value)
        elif cmd == FROM_PA_PIN_PWR_STATE:
            self.__pa_pin_pwr = int(value)
        elif cmd == FROM_PA_PTT_STATE:
            self.__pa_pin_ptt = int(value)
        elif cmd == FROM_PA_PIN_PROTECTION_STATE:
            self.__protection_pin = int(value)
        elif cmd == FROM_PA_RELAY_NUM:
            self.__relay_num = int(value)
        elif cmd == FROM_PA_EDIT_MODE:
            self.__edit_mode = int(value)
        else:
            print('Uncknown command:', data)

    def get_state(self):
        return self.__pa_pin_pwr, self.__pa_pin_ptt, self.__mech1_angle, self.__mech1_manual_mode, self.__relay_num, self.__edit_mode
