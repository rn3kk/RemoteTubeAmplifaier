#include <QTimerEvent>
#include <QHostAddress>
#include <QNetworkInterface>
#include "broadcastinformer.h"

BroadcastInformer::BroadcastInformer(QObject *parent):
  QObject(parent)
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
    QByteArray datagram;
    datagram.append("ip="+ip);
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
