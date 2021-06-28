#include <QLoggingCategory>
#include <QTcpSocket>
#include <QPair>
#include "../common/commands.h"
#include "../common/socketwrapper.h"
#include "../common/common.h"
#include "../common/backmodel.h"
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
  connect(this, &Server::sendToAllClients, sw, &SocketWrapper::writeToSocket);
  m_socketList.append(sw);
  sw->writeToSocket(BackModel::getInstance().toJson());
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
  SocketWrapper* sw = (SocketWrapper*) sender();

  Commands::Type type = Commands::getType(data);
  if(type ==  Commands::Type::CHANGE_POWER)
  {
    bool pwr = Commands::getValue(data).toInt();
    BackModel::getInstance().setPwr(pwr);
  }
  else if (type == Commands::Type::CHANGE_TUNE)
  {
    bool tune = Commands::getValue(data).toInt();
    qDebug() << "change tune to " << tune;
    BackModel::getInstance().setTuneMode(tune);
  }
  else if(type == Commands::Type::CHANGE_RELAY)
  {
    int relay = Commands::getValue(data).toInt();
    BackModel::getInstance().setRelay(relay);
  }
  else if(type == Commands::Type::CHANGE_MECH)
  {
    QPair<QString, int> p = Commands::getMechaduinoPos(data);
    BackModel::getInstance().changeMechaduino(p);
  }
  qDebug() << data;
}
