#include <QThread>
#include <QLoggingCategory>
#include <QTimerEvent>
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
  emit radioConected();
  startTimer(1000);
}

void Yaesu::setTXFreq(int freq)
{

}

void Yaesu::readyRead()
{
//  QByteArray array;
//  m_port->read(array.data(), 5);
//  qDebug()<< array;
//  qCDebug(ft857_log) << "new freq from yeasu";
//  if(!m_port || !m_port->isOpen())
//  {
//    return;
//  }
//  QByteArray a = m_port->readAll();
//  qCDebug(ft857_log) << "From YEASU " << a;
}

void Yaesu::bytesWriten(qint64 bytes)
{

}

void Yaesu::timerEvent(QTimerEvent *event)
{
  killTimer(event->timerId());

  if(!m_port || !m_port->isOpen())
  {
    return;
  }

  int count = 0;
  QByteArray array;

  array.append((unsigned char) 0x00);
  array.append((unsigned char) 0x00);
  array.append((unsigned char) 0x00);
  array.append((unsigned char) 0x00);
  array.append((unsigned char) 0x03);

  m_port->write(array);
  //m_port->flush();
  m_port->waitForBytesWritten();
  if(m_port->waitForReadyRead());
  {
    QByteArray data;
    data = m_port->readAll();
    qDebug() << "Freq " << data;
  }
  startTimer(1000);
}
