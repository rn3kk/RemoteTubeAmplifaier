# import wx

# from py_server.Window import Window
import logging
import os
import signal

from py_server.Mechaduino import Mechaduino
from py_server.PA_TCP_Server import PA_TCP_Server
from py_server.Pins import Pins
from py_server.TRX_State_TCP_Client import TRX_State_TCP_Client
from py_server.settings import Settings

PORT = 6994


def init_log():
    log_name = '/var/log/bi4pa/bi4pa.log'
    directory = os.path.dirname(log_name)
    if not os.path.exists(directory):
        os.makedirs(directory)

    log = logging.getLogger('root')
    log.setLevel(logging.DEBUG)
    file_logger = logging.FileHandler(log_name)
    file_logger.setLevel(logging.DEBUG)
    formatter = logging.Formatter(
        '%(asctime)s module:%(module)s lineno:%(lineno)d|%(message)s')
    file_logger.setFormatter(formatter)
    log.addHandler(file_logger)
    log.propagate = True
    log.info('**********Started**********')


serv = None
pins = None


def handler(signum, frame):
    global serv
    print('set terminate is True')
    if serv:
        serv.set_terminate()


def main():
    global serv
    signal.signal(signal.SIGINT, handler)
    init_log()

    s = Settings.getInstance()
    m1 = Mechaduino(s.mech1_name, s.mech1_port, s.mech1, 1)
    m1.start()

    pins = Pins()
    pins.start()
    serv = PA_TCP_Server(PORT, m1, pins)
    serv.start()

    trx_state = TRX_State_TCP_Client().getInstance()
    trx_state.add_mechaduino(mech1=m1)
    serv.join()

    trx_state.resetInstance()
    m1.set_terminate()
    m1.join()
    pins.set_terminate()
    pins.join()


if __name__ == '__main__':
    main()
