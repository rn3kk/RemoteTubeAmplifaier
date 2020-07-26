#include <QThread>
#include <QLoggingCategory>
#include <QTimerEvent>
#include <QMutexLocker>
#include "yaesu.h"

QLoggingCategory ft857_log("FT-857");

Yaesu::Yaesu(const QString comPort, QObject *parent):
  IRadio(parent),
  m_comPortName(comPort)
{
  QThread* thread = new QThread();
  this->moveToThread(thread);
  QObject::connect(thread, &QThread::started, this, &Yaesu::init, Qt::QueuedConnection);
  thread->start();
}

void Yaesu::init()
{
  m_port = new QSerialPort(m_comPortName);
  m_port->setDataBits(QSerialPort::Data8);
  m_port->setParity(QSerialPort::NoParity);
  m_port->setFlowControl(QSerialPort::NoFlowControl);
  m_port->setStopBits(QSerialPort::TwoStop);
  m_port->setBaudRate(QSerialPort::Baud9600);
  //QObject::connect(m_port, &QSerialPort::readyRead, this, &Yaesu::readyRead, Qt::QueuedConnection);
  if(!m_port->open(QSerialPort::ReadWrite))
  {
    qCritical(ft857_log) << "connection open error: "
                         << m_port->errorString();
    delete m_port;
    m_port = nullptr;
    emit radioDisconected();
    return;
  }
  if(m_port->waitForReadyRead(1000))
    m_port->readAll();
  emit radioConected();
  startTimer(1000);
}

void Yaesu::setTXFreq(int freq)
{
  QString f = QString::number(freq);
  while(f.length() <6)
  {
    f.push_front('0');
  }
  while(f.length() < 8)
  {
    f.push_back('0');
  }
  QByteArray f2 = QByteArray::fromHex(f.toUtf8());
  f2.push_back(1);

  QMutexLocker l(&m_writeToSerialPortMutex);
  if(!m_port || !m_port->isOpen())
  {
    return;
  }
  m_port->write(f2, 5);
  m_port->waitForBytesWritten();
  if(m_port->waitForReadyRead(300))
  {
    QByteArray a = m_port->readAll();
    qDebug() << "Write freq " << a.fromHex(0);
  }

}

unsigned int Yaesu::reverse(unsigned int x)
{
  x = (x & 0xFF) << 8 | (x & 0xFF00) >>  8;
  return x;
}

void Yaesu::timerEvent(QTimerEvent *event)
{
  killTimer(event->timerId());
  if(!m_port || !m_port->isOpen())
  {
    return;
  }

  QByteArray array;
  array.append((unsigned char) 0x00);
  array.append((unsigned char) 0x00);
  array.append((unsigned char) 0x00);
  array.append((unsigned char) 0x00);
  array.append((unsigned char) 0x03);

  QMutexLocker l(&m_writeToSerialPortMutex);
  m_port->write(array);
  m_port->waitForBytesWritten();
  if(m_port->waitForReadyRead());
  {
    QByteArray data(5, '0x0');
    int size  = m_port->read(data.data(), 5);
    if(size == 5 )
    {
      data.push_front(data.at(4));
      data.remove(3,3);
      int freq =  data.toHex(0).toInt();
      qDebug() << "Freq " << freq;
      emit freqChanged(freq);
    }
  }
  startTimer(300);
}
