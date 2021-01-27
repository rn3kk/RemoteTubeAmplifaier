#include <QThread>
#include <QTimerEvent>
#include <QHostAddress>
#include <QLoggingCategory>
#include "sockethandlermanager.h"
#include "sockethandler.h"

QLoggingCategory socketCat("Socket");

SocketHandler::SocketHandler()
{

}

SocketHandler::SocketHandler(QTcpSocket* socket, QObject *parent):
  QObject(parent),
  m_socket(socket)
{
  qCDebug(socketCat) << "SocketHandler()";

  m_ip = socket->peerAddress().toString();

  QThread* th = new QThread;
  this->moveToThread(th);

  connect(th, &QThread::started, this, &SocketHandler::doInit, Qt::QueuedConnection);
  connect(this, &SocketHandler::finished, this, &SocketHandler::deleteLater, Qt::QueuedConnection);
  connect(th, &QThread::finished, this, &QObject::deleteLater, Qt::QueuedConnection);
  connect(th, &QThread::finished, th, &QThread::deleteLater, Qt::QueuedConnection);
  th->start();

  connect(socket, &QTcpSocket::readyRead, this, &SocketHandler::readyRead, Qt::QueuedConnection);
  connect(socket, &QTcpSocket::disconnected, this, &SocketHandler::disconnected, Qt::QueuedConnection);
  connect(this, &SocketHandler::finished, socket, &QTcpSocket::deleteLater, Qt::QueuedConnection);
}

SocketHandler::SocketHandler(const SocketHandler &handler)
{
  m_ip = handler.m_ip;
  m_socket = handler.m_socket;
}

SocketHandler::~SocketHandler()
{
  qCDebug(socketCat) << "~SocketHandler()";
}

void SocketHandler::destroy()
{
  emit finished();
}

void SocketHandler::timerEvent(QTimerEvent *event)
{
  killTimer(event->timerId());
  if(token().isEmpty() || type() == TypeToken::UNKN)
  {
    qCCritical(socketCat) << "Timeoute waiting token from remote host";
    emit finished();
  }
}

void SocketHandler::doInit()
{
  startTimer(3000);
}

void SocketHandler::readyRead()
{
  QTcpSocket* ss =qobject_cast<QTcpSocket*>(sender());
  if(!ss)
    return;

  QByteArray data = m_socket->readAll();
  if(data.isEmpty())
    return;
  if(token().isEmpty() || type() == TypeToken::UNKN)
  {
    if(parseTokenAndClientType(data))
      SocketHandlerManager::getInstance().addSocketHandler(this);
  }
  else
    emit avaliableData(data);

}

void SocketHandler::disconnected()
{
  qCInfo(socketCat) << "Disconnected" << getTypeString() << "Token" << token();
  emit finished();
}

void SocketHandler::writeData(const QByteArray &data)
{
  if(m_socket)
    m_socket->write(data);
}

QString SocketHandler::ip() const
{
  return m_ip;
}
