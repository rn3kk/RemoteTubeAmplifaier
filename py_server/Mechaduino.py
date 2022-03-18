import logging
import threading
import time
import traceback
from datetime import datetime
from threading import Thread

import serial

log = logging.getLogger('root')


class Mechaduino(Thread):
    __serial_port = None
    __terminate = False
    __from_mech_queue = b''
    __to_mech_queue = b''
    __to_mech_queue_mutex = threading.Lock()
    __manual = False
    __last_manual_check_time = datetime.now()

    def __init__(self, name, port, freq_points):
        Thread.__init__(self)
        self.__name = name
        self.__port = port
        self.__freq_points = freq_points

    def __del__(self):
        print('~Mechaduino()', self.__name)

    def set_terminate(self):
        self.__terminate = True

    def run(self) -> None:
        print(self.__name, 'is started')
        while not self.__terminate:
            try:
                time.sleep(1)
                log.info('Probe open port ' + self.__port)
                self.__serial_port = serial.Serial(port=self.__port, baudrate=9600, parity=serial.PARITY_NONE,
                                                   timeout=0.1, bytesize=serial.EIGHTBITS,
                                                   stopbits=serial.STOPBITS_ONE, xonxoff=True, rtscts=False,
                                                   dsrdtr=False)
                log.info('Mechaduino port opened ' + str(self.__port))
            except serial.SerialException as err:
                log.error('connection to Mechaduino is failed ' + err.strerror)
                continue
            self.__serial_port.write(b'xy')
            while not self.__terminate:
                try:
                    time.sleep(0.001)
                    self.__process_from_mech_data()
                    self.__process_to_mech_data()
                    self.__request_angle_if_manual()
                except serial.SerialException as err:
                    print('connection to Mechaduino is lost' + err.strerror)
                    self.__serial_port.close()
                except Exception as err:
                    print(traceback.format_exc())
                    print('IO error ' + err.__str__())
                    break
        if self.__serial_port and self.__serial_port.isOpen():
            self.__serial_port.close()
        print(self.__name, 'is end loop')

    def __process_from_mech_data(self):
        if self.__serial_port.in_waiting > 0:
            self.__from_mech_queue += bytes(self.__serial_port.read(self.__serial_port.in_waiting))
            # print('from mech', self.__from_mech_queue)
            # self.__from_mech_queue = b''

        if not self.__from_mech_queue:
            return
        if self.__from_mech_queue.find(b';') == -1:
            return

        cmds = self.__from_mech_queue.split(b';')
        for cmd in cmds:
            if cmd.find(b'A') == 0:
                angle = round(float(cmd[2:]))
                print(angle)
        self.__from_mech_queue = b''

    def __process_to_mech_data(self):
        if self.__to_mech_queue:
            # print('->', self._to_cat_queue)
            self.__to_mech_queue_mutex.acquire()
            self.__serial_port.write(self.__to_mech_queue)
            self.__to_mech_queue = b''
            self.__to_mech_queue_mutex.release()

    def __send_to_com_port(self, data):
        self.__to_mech_queue_mutex.acquire()
        self.__to_mech_queue += data
        self.__to_mech_queue_mutex.release()

    def set_angle(self, angle=0):
        if self.__manual:
            return
        s = b'r' + str(angle).encode()
        self.__send_to_com_port(s)

    def set_manual_mode(self, mode):
        print('Manual mode is', mode)
        self.__manual = mode
        if mode is True:
            self.__send_to_com_port(b'n')
        else:
            self.__send_to_com_port(b'y')

    def __request_angle_if_manual(self):
        if self.__manual and \
                ((datetime.now() - self.__last_manual_check_time).total_seconds() * 1000 > 300.0):
            self.__send_to_com_port(b'p')
            self.__last_manual_check_time = datetime.now()
