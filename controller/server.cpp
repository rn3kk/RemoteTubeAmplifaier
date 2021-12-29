#include <QLoggingCategory>
#include <QTcpSocket>
#include <QPair>
#include "applicaionsettings.h"
#include "../common/commands.h"
#include "../common/socketwrapper.h"
#include "../common/common.h"
#include "../common/backmodel.h"
#include "MechaduinoController.h"
#include "server.h"

static QLoggingCategory srvCat("Server");

Server::Server()
{

}

Server::~Server()
{
  qDeleteAll(m_socketList);
  m_socketList.clear();
}

void Server::newConnection()
{
  QTcpSocket* socket = m_server->nextPendingConnection();
  if(!socket)
  {
    qCCritical(srvCat) << "Incomming connection. m_psm_client is null";
    return;
  }
  qCDebug(srvCat)<< "Incoming connection ";
  SocketWrapper* sw = new SocketWrapper(socket);
  connect(sw, &SocketWrapper::avaliableData, this, &Server::socketData);
  connect(sw, &SocketWrapper::socketDiskonnected, this, &Server::socketDisconnected);
  m_socketList.append(sw);
  sw->writeToSocket(BackModel::getInstance().getStatus());
}

void Server::socketDisconnected()
{
  SocketWrapper *socket = (SocketWrapper*) sender();
  socket->disconnect();
  m_socketList.removeOne(socket);
  delete socket;
}

void Server::doWork()
{
  m_server = new QTcpServer();
  QObject::connect(m_server, &QTcpServer::newConnection, this, &Server::newConnection);
  bool isListenning = m_server->listen(QHostAddress::Any, PA_TCP_PORT);
  if(!isListenning)
    qCCritical(srvCat) << "Can't run Server to listen TCP port " << PA_TCP_PORT;
  else
    qCInfo(srvCat()) << "Listennig TCP port" << PA_TCP_PORT;
}

void Server::socketData(const QByteArray &data)
{
//  SocketWrapper* sw = (SocketWrapper*) sender();

  qDebug() << data;
}
