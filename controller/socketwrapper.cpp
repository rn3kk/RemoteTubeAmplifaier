#include <QTimerEvent>
#include <QHostAddress>
#include <QLoggingCategory>
#include "socketwrapper.h"

static QLoggingCategory sockWrapper("SocketWrapper");

SocketWrapper::SocketWrapper(QObject *parent): //connected to host!
  QObject(parent)
{
  qCDebug(sockWrapper) << "SocketWrapper() " << m_name;
}

SocketWrapper::~SocketWrapper()
{
  qCDebug(sockWrapper) << "~SocketWrapper() " << m_name;
  if(m_socket)
  {
    m_socket->disconnect();
    m_socket->close();
    delete m_socket;
    m_socket = nullptr;
  }
}

void SocketWrapper::writeToSocket(const QByteArray &data)
{
  if(m_socket)
    m_socket->write(data);
}

void SocketWrapper::readyRead()
{
  QByteArray data = m_socket->readAll();
  if(!data.isEmpty())
  {
    qDebug() << data;    
    emit avaliableData(data);
  }
}

void SocketWrapper::connected()
{
  qCDebug(sockWrapper)<< "Socket " << m_name <<   " connected.";
  emit socketConnectionStatus(true);
}

void SocketWrapper::disconnected()
{
  qCDebug(sockWrapper)<< "Socket " << m_name <<   " diconnected.";
  emit socketConnectionStatus(false);
}

void SocketWrapper::connectSocketSignalsToSlots()
{
  if(!m_socket) return;
  QObject::connect(m_socket, &QTcpSocket::readyRead, this, &SocketWrapper::readyRead, Qt::QueuedConnection);
  QObject::connect(m_socket, &QTcpSocket::stateChanged, this, &SocketWrapper::stateChanged, Qt::QueuedConnection);
  QObject::connect(m_socket, &QTcpSocket::connected, this, &SocketWrapper::connected, Qt::QueuedConnection);
  QObject::connect(m_socket, &QTcpSocket::disconnected, this, &SocketWrapper::disconnected, Qt::QueuedConnection);
}
