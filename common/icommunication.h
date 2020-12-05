#ifndef IPACKETSOURCE_H
#define IPACKETSOURCE_H

#include <QObject>
#include "socketwrapper.h"

class ICommunication
{  
public:
  virtual ~ICommunication()
  {
    if(m_socket)
      delete m_socket;
  }

  virtual void outcomingPacket(const Packet& packet) = 0;
  virtual void incomingPacket(const Packet& packet) = 0;
  virtual void socketData(const QByteArray& data) = 0;

protected:
  SocketWrapper *m_socket;
};

#endif // IPACKETSOURCE_H
