#include <QSerialPort>
#include <QThread>
#include <QLoggingCategory>
#include "MechaduinoCommunicator.h"

QLoggingCategory mechCat("MechaduinoCommunicator");

MechaduinoCommunicator::MechaduinoCommunicator(const QString &comPortName, QObject *parent):
  QObject(parent),
  m_comPortName(comPortName)
{
  m_port = new QSerialPort(comPortName);
  m_port->setBaudRate(QSerialPort::Baud115200);
  m_port->setDataBits(QSerialPort::Data8);
  m_port->setParity(QSerialPort::NoParity);
  m_port->setFlowControl(QSerialPort::NoFlowControl);
  m_port->setStopBits(QSerialPort::TwoStop);
  m_port->setBaudRate(QSerialPort::Baud115200);
  if(!m_port->open(QSerialPort::ReadWrite))
  {
    qCritical(mechCat) << "connection open error: "
                       << m_port->errorString();
    delete m_port;
    m_port = nullptr;
  }
}

MechaduinoCommunicator::~MechaduinoCommunicator()
{
  if(m_port)
  {
    m_port->close();
    delete m_port;
  }
}

float MechaduinoCommunicator::getPosition()
{
  if(m_port->isOpen())
  {
    m_port->readAll();
    m_port->write("p");
    m_port->waitForBytesWritten();
    m_port->waitForReadyRead();
    QString position = m_port->readAll();
    qDebug(mechCat) << position;
    m_port->write("xy");
  }
}

void MechaduinoCommunicator::setPosition(int newPosition)
{
  if(m_port->isOpen())
  {
    m_port->waitForReadyRead();
    m_port->readAll();
    m_port->write("r");
    m_port->write(QString::number(newPosition).toStdString().c_str());
    m_port->waitForBytesWritten();
    m_port->waitForReadyRead();
    QString position = m_port->readAll();
    qDebug(mechCat) << position;
  }
  thread()->msleep(100);
}
