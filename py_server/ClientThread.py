import json
import logging
import time
from datetime import datetime

import errno
import socket
import sys
import traceback
import re
from threading import Thread

from common.common import *

log = logging.getLogger('root')

PORT_AUDIO = 6992



class ClientThread(Thread):
    __terminate = False

    def __init__(self, conn, addr, mech1, pins):
        log.debug('ClientControlThread()')
        Thread.__init__(self)
        self.__conn = conn
        self.__set_keep_alive(self.__conn)
        self.__conn.settimeout(0.1)
        self.__autorisation_token = ''
        self.__autorised = False
        self.__connection_time = datetime.now()
        self.__addr = addr
        self.__mech1 = mech1
        self.__pins = pins


    def __del__(self):
        log.debug('~ClientControlThread()')

    def run(self):
        log.debug('ClientControlThread is run')
        while not self.__terminate:
            time.sleep(0.001)
            if not self.__autorised \
                    and ((datetime.now() - self.__connection_time).total_seconds() * 1000 > 1000):
                log.error('client autorisation error')
                self.__conn.close()
                self.__client_disconnected()
                break
            try:
                data = self.__conn.recv(128)
                if self.__terminate:
                    break
                if len(data) == 0:
                    self.__conn.close()
                    self.__client_disconnected()
                    break
                print('receive', data)
                s = re.compile(b"{.*?}")
                m = s.search(data)
                while m:
                    try:
                        data = data.replace(m.group(), b'')
                        cmd = json.loads(m.group())
                        if self.__autorised:
                            c = cmd[COMMAND]
                            if c == CMD_CHANGE_ANGLE_MECH1:
                                angle = int(cmd[VALUE])
                                self.__mech1.set_angle(angle)
                            elif c == CMD_CHANGE_ANGLE_MECH2:
                                angle = cmd[VALUE]
                                self.__mech1.set_angle(angle)
                            elif c == CMD_CHANGE_MANUAL_MECH1:
                                self.__mech1.set_manual_mode()
                            elif c == CMD_CHANGE_PWR:
                                self.__pins.change_pwr()
                        elif cmd[COMMAND] == CMD_AUTORISATION_TOKEN:
                            print('to server autoorised')
                            self.__autorisation_token = cmd[VALUE]
                            self.__cleint_autorised()
                            log.info('client autorisation OK')
                        else:
                            log.warning('Unknown command ' + data.decode())

                    except:
                        print('error in data', data)
                    finally:
                        m = s.search(data)

            except socket.timeout:
                pass
            except Exception:
                print(traceback.format_exc())
                self.__conn.close()
                self.__conn = None
                log.error('close connection')
                self.__client_disconnected()
                break

            try:
                if self.__autorised:
                    pins_d = self.__pins.get_data()
                    if pins_d:
                        print('Send_pins',pins_d)
                        self.__conn.send(pins_d)
                    mech1_d = self.__mech1.get_data()
                    if mech1_d:
                        print('Send_mech', mech1_d)
                        self.__conn.send(mech1_d)


            except socket.error as e:
                log.error('socket.error' + str(e))
                if e.errno == errno.ECONNRESET:
                    self.__conn.close()
                    self.__conn = None
                    log.error('close connection')
                    self.__client_disconnected()
                    break
                else:
                    raise
            except socket.timeout:
                pass
        if self.__conn:
            self.__conn.close()

        print('ClientControlThread loop is end')

    def __cleint_autorised(self):
        addr = self.__addr
        log.info('input connection from  ' + addr[0]+ ' autorised')
        self.__autorised = True
        self.__pins.client_connected()
        self.__mech1.client_connected()


    def __client_disconnected(self):
        addr = self.__addr
        log.info('disconnected ' + addr[0])

    def __set_keep_alive(self, sock, after_idle_sec=1, interval_sec=5, max_fails=6):
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, after_idle_sec)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, interval_sec)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, max_fails)

    def set_terminate(self):
        self.__terminate = True




