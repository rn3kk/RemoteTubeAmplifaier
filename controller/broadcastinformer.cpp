#include <QTimerEvent>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QDateTime>
#include "broadcastinformer.h"

BroadcastInformer::BroadcastInformer(QObject *parent):
  QObject(parent)
{

}

BroadcastInformer::~BroadcastInformer()
{

}

void BroadcastInformer::start()
{
  m_socket = new QUdpSocket();
  startTimer(3000);
}

void BroadcastInformer::timerEvent(QTimerEvent *event)
{
  killTimer(event->timerId());
  QString ip = getIp();
  if(!ip.isEmpty() && !ip.isEmpty())
  {
    QDateTime dt = QDateTime::currentDateTime();
    QByteArray datagram;
    datagram.append("ip="+ip);
    datagram.append("time=" + dt.toString("dd.MM.yyyy hh:mm:ss.zzz"));
    if(m_socket)
      m_socket->writeDatagram(datagram, QHostAddress::Broadcast, 5992);
  }
  startTimer(3000);
}

QString BroadcastInformer::getIp() const
{
  const QHostAddress &lockalHost=QHostAddress(QHostAddress::LocalHost);
  for(const QHostAddress &address: QNetworkInterface::allAddresses())
  {
    if(address.protocol() == QAbstractSocket::IPv4Protocol &&
       address != lockalHost)
      return address.toString();
  }
  return QString();
}
