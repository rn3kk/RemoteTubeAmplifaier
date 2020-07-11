#include <QThread>
#include <QLoggingCategory>
#include "ft857.h"

QLoggingCategory ft857_log("FT-857");

ft857::ft857(const QString comPort, QObject *parent):
  IRadio(parent),
  m_comPortName(comPort)
{
  QThread* thread = new QThread();
  this->moveToThread(thread);
  QObject::connect(thread, &QThread::started, this, &ft857::init, Qt::QueuedConnection);
  thread->start();
}

void ft857::init()
{
  m_port = new QSerialPort(m_comPortName);
  m_port->setDataBits(QSerialPort::Data8);
  m_port->setParity(QSerialPort::OddParity);
  m_port->setFlowControl(QSerialPort::SoftwareControl);
  m_port->setStopBits(QSerialPort::OneStop);
  m_port->setBaudRate(QSerialPort::Baud38400);
  QObject::connect(m_port, &QSerialPort::readyRead, this, &ft857::readyRead, Qt::QueuedConnection);
  if(!m_port->open(QSerialPort::ReadWrite))
  {
    qCritical(ft857_log) << "connection open error: "
                       << m_port->errorString();
    delete m_port;
    m_port = nullptr;
    emit radioDisconected();
  }
  emit radioConected();
}

void ft857::setTXFreq(int freq)
{

}

void ft857::readyRead()
{

}

void ft857::bytesWriten(qint64 bytes)
{

}
