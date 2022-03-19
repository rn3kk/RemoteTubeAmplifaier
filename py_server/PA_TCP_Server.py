import socket
import time
from threading import Thread, Lock

import logging

from py_server.ClientThread import ClientThread

log = logging.getLogger('root')


class PA_TCP_Server(Thread):
    __terminate = False
    __data = b''
    __port = 6991
    __last_client = None

    def __init__(self, port, mech1, pins):
        log.debug('PA_TCP_Server()')
        Thread.__init__(self)
        self.__port = port
        self.__mech1 = mech1
        self.__pins = pins

    def __del__(self):
        print('~PA_TCP_Server()')


    def run(self):
        log.debug('PA_TCP_Server is run')
        while not self.__terminate:
            time.sleep(1)
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            sock.settimeout(5)
            try:
                sock.bind(('', self.__port))
            except:
                log.error('ERROR bind port', self.__port)
                time.sleep(5.0)
                continue
            sock.listen(5)
            # log.info('socket start accept port ' + str(self.__port))
            try:
                conn, addr = sock.accept()
            except socket.timeout:
                continue
            if self.__last_client:
                self.__last_client.set_terminate()
                self.__last_client.join()
            self.__last_client = ClientThread(conn, addr, self.__mech1, self.__pins)

            self.__last_client.start()
            time.sleep(0.1)
        if self.__last_client:
            self.__last_client.set_terminate()
            self.__last_client.join()

        log.debug('PA_TCP_Server loop is end')

    def set_terminate(self):
        self.__terminate = True
