#ifndef REMOTECLIENTSOCKETWRAPPER_H
#define REMOTECLIENTSOCKETWRAPPER_H

#include "sockethandler.h"

class RemoteClientSocketWrapper: public SocketHandler
{
public:
  RemoteClientSocketWrapper();
  RemoteClientSocketWrapper(QTcpSocket* socket);
};

Q_DECLARE_METATYPE(RemoteClientSocketWrapper)

#endif // REMOTECLIENTSOCKETWRAPPER_H
