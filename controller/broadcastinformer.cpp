#include <QTimerEvent>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QDateTime>
#include "applicaionsettings.h"
#include "../common/ampinfo.h"
#include "../common/common.h"
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
    QString ampName = ApplicaionSettings::getInstance().getPaName();
    AmpInfo ai(ampName, ip);
    if(m_socket)
      m_socket->writeDatagram(ai.toXml(), QHostAddress::Broadcast, PA_UDP_BROADCAST_PORT);
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
