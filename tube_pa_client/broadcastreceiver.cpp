#include <QNetworkDatagram>
#include <QDebug>
#include "broadcastreceiver.h"

BroadcastReceiver::BroadcastReceiver(QObject *parent) : QObject(parent)
{

}

void BroadcastReceiver::start()
{
  m_socket = new QUdpSocket();
  m_socket->bind(QHostAddress::Broadcast, 5992);

  connect(m_socket, &QUdpSocket::readyRead, this, &BroadcastReceiver::readyRead, Qt::QueuedConnection);

}

void BroadcastReceiver::readyRead()
{
  m_socket->open(QIODevice::ReadOnly);
  QByteArray data = m_socket->readAll();
  qDebug() << data;

//  QNetworkDatagram datagramm = m_socket->receiveDatagram();
//  if(!datagramm.data().isEmpty())
//  {
//    qDebug() << datagramm.data();
//    QNetworkDatagram forWrite = datagramm.makeReply(datagramm.data());
//    m_socket->writeDatagram(forWrite);
//  }


}
