#include <QLoggingCategory>
#include <QTcpSocket>
#include "socketwrapper.h"
#include "server.h"

static QLoggingCategory psmServerCat("PsmServer");

Server::Server(quint16 port):
  m_listeningPort(port)
{

}

Server::~Server()
{
  if(m_socket)
  {
    m_socket->disconnect();
    delete m_socket;
  }
}

void Server::newConnection()
{
  if(m_socket)
  {        
    delete m_socket;
    m_socket = nullptr;
  }

  QTcpSocket* socket = m_server->nextPendingConnection();
  if(!socket)
  {
    qCCritical(psmServerCat) << "Incomming connection. m_psm_client is null";
    return;
  }
  m_socket = new SocketWrapper(socket);
  QObject::connect(m_socket, &SocketWrapper::avaliableData, this, &Server::socketData, Qt::QueuedConnection);
  //QObject::connect(m_socket, &SocketWrapper::socketConnectionStatus, this, &Server::serverConnectionStatus, Qt::QueuedConnection);
  //emit serverConnectionStatus(true);//сокет уже в подключенном сосотоянии
  qCDebug(psmServerCat)<< "Incoming connection ";
}

void Server::timerEvent(QTimerEvent *event)
{
  if(m_socket)
  {
    m_socket->disconnect();
    delete m_socket;
    m_socket = nullptr;
  }
}

void Server::doWork()
{
  m_socket = nullptr;
  m_server = new QTcpServer();
  QObject::connect(m_server, &QTcpServer::newConnection, this, &Server::newConnection, Qt::QueuedConnection);
  bool isListenning = m_server->listen(QHostAddress::Any, m_listeningPort);
  if(!isListenning)
    qCCritical(psmServerCat) << "Can't run PsmServer to listen port " << m_listeningPort;
  else
    qCInfo(psmServerCat()) << "Listennigg port " << m_listeningPort;
}

void Server::socketData(const QByteArray &data)
{
  qCDebug(psmServerCat) << "PSM Socket data " << data;



  //Packet p(m_ci.name(), Packet::Source::SERVER, data);
  //emit outcomingPacket(p);
}
