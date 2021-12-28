#ifndef COMMON_H
#define COMMON_H

#include <QString>

const unsigned int PA_TCP_PORT = 5993;
const unsigned int PA_UDP_BROADCAST_PORT = 5992;

const QString CMD = "c";
const QString DATA = "d";


const QString TOKEN = "token";
const QString REQUEST = "request";
const QString CHANGE_REUQEST = "change";
const QString REQUEST_GET_INFO = "get-info";
const QString KEY = "key";
const QString VALUE = "v";

const QString CHANGE_POWER = "change-power";
const QString CHANGE_TUNE = "change-tune";
const QString CHANGE_MECH = "change-mech";
const QString CHANGE_RELAY = "change-relay";

const QString MODEL = "model";
const QString DATA_TYPE = "data_type";

const QString FREQ = "f";
const QString POWER = "p";
const QString RELAY = "r";
const QString MECH = "mechaduino";
const QString SEPARATOR = ";";

const int CMD_STATUS = 1;
const int CMD_PWR = 2;
const int CMD_MECH1 = 3;
const int CMD_MECH2 = 4;
const int CMD_TUNE_MODE = 5;
const int CMD_MANUAL_MODE = 6;
const int CMD_RELAY = 7;

#endif // COMMON_H
