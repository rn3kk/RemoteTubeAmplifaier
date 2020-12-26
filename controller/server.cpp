#include <QLoggingCategory>
#include <QTcpSocket>
#include "jsonprotokol.h"
#include "socketwrapper.h"
#include "server.h"

static QLoggingCategory srvCat("Server");

Server::Server(quint16 port):
  m_listeningPort(port)
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
  bool isListenning = m_server->listen(QHostAddress::Any, m_listeningPort);
  if(!isListenning)
    qCCritical(srvCat) << "Can't run Server to listen port " << m_listeningPort;
  else
    qCInfo(srvCat()) << "Listennigg port " << m_listeningPort;
}

void Server::socketData(const QByteArray &data)
{
  SocketWrapper* sw = (SocketWrapper*) sender();
  if(!JsonProtokol::checkToken(data))
  {
    m_socketList.removeOne(sw);
    delete sw;
  }
  qCDebug(srvCat) << "Socket data " << data;
}
