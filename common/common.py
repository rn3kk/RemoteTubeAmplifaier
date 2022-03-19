import io
import json

def is_raspberrypi():
    try:
        with io.open('/sys/firmware/devicetree/base/model', 'r') as m:
            if 'raspberry pi' in m.read().lower(): return True
    except Exception:
        pass
    return False


RASPBERRY = is_raspberrypi()

COMMAND = "c"
VALUE = "v"

CMD_AUTORISATION_TOKEN = 1
CMD_CHANGE_PWR = 2
CMD_CHANGE_RELAY = 3
CMD_CHANGE_ANGLE_MECH1 = 4
CMD_CHANGE_ANGLE_MECH2 = 5
CMD_CHANGE_PWR_PIN = 6

FROM_PA_MANUAL_MODE = 1
FROM_PA_ANGLE_MECH1 = 2
FROM_PA_ANGLE_MECH2 = 3
FROM_PA_PIN_PWR_STATE = 4
FROM_PA_PIN_PROTECTION_STATE = 5
FROM_PA_PTT_STATE = 6


class Protocol:

    @staticmethod
    def parseCmd(data):
        if type(data) is str or type(data) is bytes:
            json_cmd = json.loads(data)
        else:
            json_cmd = data
        cmd = json_cmd[COMMAND]
        value = str(json_cmd[VALUE])
        return cmd, value

    @staticmethod
    def createCmd(cmd, value):
        data = {COMMAND: cmd, VALUE: str(value)}
        return json.dumps(data).encode()

