#include <QLoggingCategory>
#include <QTcpSocket>
#include "statemodel.h"
#include "jsonprotokol.h"
#include "../common/socketwrapper.h"
#include "../common/common.h"
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
  SocketWrapper* sw = new SocketWrapper(socket);
  QObject::connect(sw, &SocketWrapper::avaliableData, this, &Server::socketData, Qt::QueuedConnection);
  QObject::connect(sw, &SocketWrapper::socketDiskonnected, this, &Server::socketDisconnected, Qt::QueuedConnection);
  m_socketList.append(sw);
  qCDebug(srvCat)<< "Incoming connection ";
}

void Server::socketDisconnected()
{
  SocketWrapper *socket = (SocketWrapper*) sender();
  m_socketList.removeOne(socket);
  delete socket;
}

void Server::doWork()
{
  m_server = new QTcpServer();
  QObject::connect(m_server, &QTcpServer::newConnection, this, &Server::newConnection, Qt::QueuedConnection);
  bool isListenning = m_server->listen(QHostAddress::Any, PA_TCP_PORT);
  if(!isListenning)
    qCCritical(srvCat) << "Can't run Server to listen TCP port " << PA_TCP_PORT;
  else
    qCInfo(srvCat()) << "Listennig TCP port" << PA_TCP_PORT;
}

void Server::socketData(const QByteArray &data)
{
  SocketWrapper* sw = (SocketWrapper*) sender();
//  if(!JsonProtokol::checkToken(data))
//  {
//    m_socketList.removeOne(sw);
//    delete sw;
//  }
  if(JsonProtokol::isRequest(data))
    sw->writeToSocket(StateModel::getInstance().toJson());
  qCDebug(srvCat) << "Socket data " << data;
}
