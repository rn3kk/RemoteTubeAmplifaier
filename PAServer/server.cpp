#include <QThread>
#include <QLoggingCategory>

#include "server.h"

QLoggingCategory serverCat("Server");

Server::Server(QObject *parent):
  QObject(parent)
{
  QThread* th = new QThread();
  this->moveToThread(th);
  QObject::connect(th, &QThread::started, this, &Server::doInit, Qt::QueuedConnection);
  th->start();
}

void Server::doInit()
{
  m_server = new QTcpServer;
  connect(m_server, &QTcpServer::newConnection, this, &Server::newConnection, Qt::QueuedConnection);
  if(m_server->listen(QHostAddress::Any, 6789))
    qCInfo(serverCat) << "Listennig port 6789";
  else
    qCCritical(serverCat) << "Can't open port 6789";
}

void Server::newConnection()
{
  qCDebug(serverCat) << "Incoming connection";

 QTcpSocket* socket = m_server->nextPendingConnection();
 SocketHandler* handler = new SocketHandler(socket);
 //SocketHandlerManager::getInstance().addSocketHandler(handler);
}
