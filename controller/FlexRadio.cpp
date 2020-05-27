#include <QThread>
#include <QLoggingCategory>
#include <QTimerEvent>
#include "FlexRadio.h"

//LIST = b'C41|slice list\r\n'
//SUB_SCU = b'C21|sub scu all\r\n'
//SUB_SLICE = b'C21|sub slice all\r\n'
const QByteArray SUB_SLICE = "C21|sub slice all\r\n";
//const QByteArray SUB_SLICE = "sub slice all \r\n";
const QByteArray SUB_TX_ALL = "sub tx all";
const QByteArray SUB_FOUNDATION = "C21|sub foundation all";
const QString SLICE = "slice";
const QString RF_FREQENCY = "RF_frequency";

QLoggingCategory radioLog("FlexRadio");

FlexRadio::FlexRadio(const QString &host, quint16 port,  QObject* parent):
  QObject(parent),
  m_radioHost(host),
  m_radioPort(port),
  m_socket(new QTcpSocket(this))
{
  connect(m_socket, &QTcpSocket::readyRead, this, &FlexRadio::readData, Qt::QueuedConnection);
  connect(m_socket, &QTcpSocket::connected, this, &FlexRadio::isConnected, Qt::QueuedConnection);
  connect(m_socket, &QTcpSocket::disconnected, this, &FlexRadio::isDisconected, Qt::QueuedConnection);
  connect(m_socket, &QTcpSocket::stateChanged, this, &FlexRadio::socketChangeState, Qt::QueuedConnection);
  connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &FlexRadio::socketError, Qt::QueuedConnection);
  connect(m_socket, &QTcpSocket::bytesWritten, this, &FlexRadio::writen, Qt::QueuedConnection);
  m_socket->connectToHost(m_radioHost, m_radioPort);
}

void FlexRadio::isConnected()
{
  qCDebug(radioLog) << "Connect to host";
  //startTimer(1000);

  QByteArray array;
  m_socket->waitForReadyRead();
  array = m_socket->readAll();
  QList<QByteArray> list = array.split('\n');

  qCDebug(radioLog) << array;

  m_remoteVersionProtocol = list.at(0);
  m_handleSeq = list.at(1);

  m_socket->write(SUB_SLICE);
  m_socket->waitForBytesWritten();
}

void FlexRadio::isDisconected()
{
  qCDebug(radioLog) << "Disconnect from host";
}

QByteArray array;
QList<QByteArray>::const_iterator it;
void FlexRadio::readData()
{
  qint64 bytes = m_socket->bytesAvailable();
  array = m_socket->read(bytes);
  QList<QByteArray> list = array.split('|');
  for(it=list.constBegin(); it != list.constEnd(); ++it)
  {
    if(it->indexOf(SLICE) == 0)
    {
      parseVfomSLICE((*it));
    }
  }
  //qCDebug(radioLog) << "Read from socket " << bytes;
  //qDebug() << array;

  //emit radioFrequency(array.toUInt());
  //emit radioFrequency(7100000);
}

void FlexRadio::socketChangeState(QAbstractSocket::SocketState socketState)
{
  qDebug(radioLog) << "Socket change state to " << socketState;
}

void FlexRadio::socketError(QAbstractSocket::SocketError error)
{
  switch (error)
  {
  case QAbstractSocket::AddressInUseError:
    qCCritical(radioLog) <<  "SOCKET ERROR: Address is already in use";
    break;
  case QAbstractSocket::ConnectionRefusedError:
    qCCritical(radioLog) <<  "SOCKET ERROR: Connection refused";
    break;
  case QAbstractSocket::HostNotFoundError:
    qCCritical(radioLog) <<  "SOCKET ERROR: Host not found";
    break;
  case QAbstractSocket::RemoteHostClosedError:
    qCCritical(radioLog) <<  "SOCKET ERROR: Remote host closed";
    break;
  }

  m_socket->abort();
  m_socket->close();
  thread()->sleep(1);
  m_socket->connectToHost(m_radioHost, m_radioPort);
}

void FlexRadio::writen(qint64 bytes)
{
  qCDebug(radioLog) << "Write to socket " << bytes << " bytes";
}

void FlexRadio::timerEvent(QTimerEvent *event)
{
  killTimer(event->timerId());
  if(m_socket== nullptr)
    return;
  if(m_socket->state() == QTcpSocket::ConnectedState)
  {
    m_socket->write(SUB_SLICE, SUB_SLICE.length());
  }
  startTimer(1000);
}

QList<QByteArray> sliceData;
QList<QByteArray>::const_iterator i;
void FlexRadio::parseVfomSLICE(const QByteArray &data)
{
  sliceData = data.split(' ');
  int sliceNumber = sliceData.at(1).toInt();
  for(i=sliceData.constBegin(); i !=sliceData.constEnd(); ++i)
  {
    if( i->indexOf(RF_FREQENCY) == 0)
    {
      float freq= i->split('=').at(1).toFloat();
      qDebug()<< "Radio freq " << freq;
    }
  }
}
