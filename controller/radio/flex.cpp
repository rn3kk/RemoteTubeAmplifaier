#include <QThread>
#include <QLoggingCategory>
#include <QTimerEvent>
#include "flex.h"

//LIST = b'C41|slice list\r\n'
//SUB_SCU = b'C21|sub scu all\r\n'
//SUB_SLICE = b'C21|sub slice all\r\n'
const QByteArray SUB_SLICE = "C21|sub slice all\r\n";
//const QByteArray SUB_SLICE = "sub slice all \r\n";
const QByteArray SUB_TX_ALL = "sub tx all";
const QByteArray SUB_FOUNDATION = "C21|sub foundation all";
const QString SLICE = "slice";
const QString RF_FREQENCY = "RF_frequency=";
const QString ACTIVE = "active=";
const QString TX = "tx=";
const QByteArray N = "\\n";

QLoggingCategory radioLog("FlexRadio");

Flex::Flex(const QString &host, quint16 port,  QObject* parent):
  IRadio(parent),
  m_radioHost(host),
  m_radioPort(port),
  m_socket(new QTcpSocket(this))
{
  connect(m_socket, &QTcpSocket::readyRead, this, &Flex::readData, Qt::QueuedConnection);
  connect(m_socket, &QTcpSocket::connected, this, &Flex::isConnected, Qt::QueuedConnection);
  connect(m_socket, &QTcpSocket::disconnected, this, &Flex::isDisconected, Qt::QueuedConnection);
  connect(m_socket, &QTcpSocket::stateChanged, this, &Flex::socketChangeState, Qt::QueuedConnection);
  connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &Flex::socketError, Qt::QueuedConnection);
  connect(m_socket, &QTcpSocket::bytesWritten, this, &Flex::writen, Qt::QueuedConnection);
  m_socket->connectToHost(m_radioHost, m_radioPort);
}

void Flex::setTXFreq(int freq)
{
  double d = freq / 1000.;
  QString data = "C12|slice t ";
  if(m_A_isTX)
  {
    data.append(" 0 ");
  }
  else if(m_B_isTX)
  {
    data.append(" 1 ");
  }
  else
  {
    data = QString();
  }
  if (!data.isNull() && !data.isEmpty())
  {
    data.append(QString::number(d));
    data.append("\n");
    qDebug() << data;
    m_socket->write(data.toLatin1());
    m_socket->waitForBytesWritten();   
    emit freqChanged(freq);
  }
}

void Flex::isConnected()
{
  qCDebug(radioLog) << "Connect to host";
  //startTimer(1000);

  QByteArray array;
  m_socket->waitForReadyRead();
  array = m_socket->readAll();
  QList<QByteArray> list = array.split('\n');

  qCDebug(radioLog) << array;

  m_remoteVersionProtocol = list.at(0);
  //m_handleSeq = list.at(1);

  m_socket->write(SUB_SLICE);
  m_socket->waitForBytesWritten();
}

void Flex::isDisconected()
{
  qCDebug(radioLog) << "Disconnect from host";
  emit freqChanged(-1);
}

QByteArray array;
QList<QByteArray>::const_iterator it;
void Flex::readData()
{
  qint64 bytes = m_socket->bytesAvailable();
  array = m_socket->read(bytes);
  qCDebug(radioLog) << "Read from socket " << bytes;
  qDebug() << array;
  QList<QByteArray> list = array.split('|');
  for(it=list.constBegin(); it != list.constEnd(); ++it)
  {
    if(it->indexOf(SLICE) == 0)
    {
      //TODO отрезать \n  все что после него, иначе не верно парсится
      parseVfomSLICE(*it);
    }
  }

  //emit radioFrequency(array.toUInt());
  //emit radioFrequency(7100000);
}

void Flex::socketChangeState(QAbstractSocket::SocketState socketState)
{
  qDebug(radioLog) << "Socket change state to " << socketState;
}

void Flex::socketError(QAbstractSocket::SocketError error)
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

void Flex::writen(qint64 bytes)
{
  qCDebug(radioLog) << "Write to socket " << bytes << " bytes";
}

void Flex::timerEvent(QTimerEvent *event)
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
QByteArray a;
int freq;
void Flex::parseVfomSLICE(const QByteArray &data)
{
  int pos = data.indexOf('\n');
  if(pos)
    sliceData = data.left(pos).split(' ');
  else
    sliceData = data.split(' ');

  int sliceNumber = sliceData.at(1).toInt();
  quint8 active;
  quint8 tx;
  for(i=sliceData.constBegin(); i !=sliceData.constEnd(); ++i)
  {
    if( i->indexOf(RF_FREQENCY) == 0)
    {
      freq= i->split('=').at(1).toFloat()*1000;
      if(sliceNumber == 0)
        m_slice1Freq = freq;
      else if(sliceNumber == 1)
        m_slice2Freq = freq;
      qDebug()<< "Slice "<< sliceNumber << "Radio freq " << freq;
    }
    else if(i->indexOf(ACTIVE) == 0)
    {
      active = i->split('=').at(1).toInt();
      if(sliceNumber == 0)
        m_activeSlice1= active;
      else if(sliceNumber == 1)
        m_activeSlice2 = active;
    }
    else if(i->indexOf(TX) == 0)
    {
      tx = i->split('=').at(1).toInt();
      if(sliceNumber == 0)
      {
        m_A_isTX = tx;
      }
      else if(sliceNumber == 1)
      {
        m_B_isTX=tx;
      }
    }
  }
  if(!m_A_isTX && !m_B_isTX)
    Q_EMIT freqChanged(-1);
  else if(m_A_isTX)
    Q_EMIT freqChanged(m_slice1Freq);
  else if(m_B_isTX)
    Q_EMIT freqChanged(m_slice2Freq);
  qDebug()<< "Slice "<< sliceNumber << " Active "<< active <<" Radio freq " << freq;
}
