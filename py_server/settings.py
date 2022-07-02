from configparser import ConfigParser

MECH1 = 'mech1'
MECH2 = 'mech2'
PORT = 'port'
IP = 'ip'
RELAY= 'relay'
STEP = 'step'
NAME = 'Name'
TRX= 'trx'
CONFIG_FILE_NAME = 'server_config.ini'


class Settings:
    __instance = None
    __start_freq = 1700
    __end_freq = 30000
    __step_freq = 25
    __max_relay = 8
    __mech1_port = '/dev/ttyACM0'
    __mech2_port = '/dev/ttyACM1'
    __mech1_name = 'tune'
    __mech2_name = 'load'
    __trx_ip = '127.0.0.1'
    __trx_port = 6994

    def __init__(self):
            self.__relay = {}
            self.__mech1 = {}
            self.__mech2 = {}
            if not Settings.__instance:
                print('Settings')
                i = self.__start_freq
                while i < self.__end_freq + self.__step_freq:
                    self.__relay[i] = 0
                    self.__mech1[i] = 0
                    self.__mech2[i] = 0
                    i += self.__step_freq

                self.__config = ConfigParser()
                self.__config.read(CONFIG_FILE_NAME)
                if not self.__config.sections():
                    print('Config file not found. Save default config file')
                    self.save_settings()  # save empty settings
                else:
                    print('Config file found. Read settings')
                    self.__read_settings()



    @classmethod
    def getInstance(cls):
        if not cls.__instance:
            cls.__instance = Settings()
        return cls.__instance

    @classmethod
    def resetInstance(cls):
        if cls.__instance:
            cls.__instance.__del__()

    def __new__(cls, *args, **kwargs):
        if not hasattr(cls, 'instance'):
            cls.instance = super(Settings, cls).__new__(cls)
        return cls.instance

    def __del__(self):
        print('~Settings')

    def __read_settings(self):
        self.__trx_ip = self.__config.get(TRX, IP)
        self.__trx_port = int(self.__config.get(TRX, PORT))
        self.__mech1_port = self.__config.get(MECH1, PORT)
        self.__mech1_name = self.__config.get(MECH1, NAME)
        self.__mech2_port = self.__config.get(MECH2, PORT)
        self.__mech2_name = self.__config.get(MECH2, NAME)

        i = self.__start_freq
        while i < self.__end_freq+self.__step_freq:
            try:
                m = self.__config.get(MECH1, str(i))
                self.__mech1[i] = int(m)
            except:
                self.__mech1[i] = 0

            try:
                m = self.__config.get(MECH2, str(i))
                self.__mech2[i] = int(m)
            except:
                self.__mech2[i] = 0

            try:
                m = self.__config.get(RELAY, str(i))
                self.__relay[i] = int(m)
            except:
                self.__relay[i] = 0

            i += self.__step_freq

    def save_settings(self):
        try:
            self.__config.add_section(TRX)
        except:
            pass
        self.__config.set(TRX, IP, self.__trx_ip)
        self.__config.set(TRX, PORT, str(self.__trx_port))
        try:
            self.__config.add_section(MECH1)
        except:
            pass
        self.__config.set(MECH1, PORT, self.__mech1_port)
        self.__config.set(MECH1, NAME, self.__mech1_name)
        for i in self.__mech1:
            self.__config.set(MECH1, str(i), str(self.mech1[i]))

        try:
            self.__config.add_section(MECH2)
        except:
            pass
        self.__config.set(MECH2, PORT, self.__mech2_port)
        self.__config.set(MECH2, NAME, self.__mech2_name)
        for i in self.__mech2:
            self.__config.set(MECH2, str(i), str(self.mech2[i]))

        try:
            self.__config.add_section(RELAY)
        except:
            pass
        for i in self.__relay:
            self.__config.set(RELAY, str(i), str(self.__relay[i]))





        try:
            f = open(CONFIG_FILE_NAME, 'w')
            self.__config.write(f)
        except Exception as e:
            print(e.with_traceback())
        finally:
            f.close()

    @property
    def mech1(self):
        return self.__mech1

    @property
    def mech2(self):
        return self.__mech2

    @property
    def mech1_name(self):
        return self.__mech1_name

    @property
    def mech2_name(self):
        return self.__mech2_name

    @property
    def mech1_port(self):
        return self.__mech1_port

    @property
    def mech2_port(self):
        return self.__mech2_port

    @property
    def trx_addr(self):
        return (self.__trx_ip, self.__trx_port)




