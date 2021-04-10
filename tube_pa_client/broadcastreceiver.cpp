#include <QNetworkDatagram>
#include <QDebug>
#include "../common/ampinfo.h"
#include "../common/common.h"
#include "broadcastreceiver.h"

BroadcastReceiver::BroadcastReceiver(QObject *parent) : QObject(parent)
{

}

void BroadcastReceiver::start()
{
  m_socket = new QUdpSocket();
  m_socket->bind(QHostAddress::Broadcast, PA_UDP_BROADCAST_PORT);
  connect(m_socket, &QUdpSocket::readyRead, this, &BroadcastReceiver::readyRead);
}

void BroadcastReceiver::readyRead()
{
  QByteArray datagram;
  while (m_socket->hasPendingDatagrams())
  {
    datagram.resize(int(m_socket->pendingDatagramSize()));
    m_socket->readDatagram(datagram.data(), datagram.size());
    AmpInfo ai(datagram);
    if(ai.isValid())
      emit findAmplifaier(ai);


    //qDebug() << datagram;
  }
}
