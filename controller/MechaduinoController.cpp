#include <QSerialPort>
#include <QThread>
#include <QLoggingCategory>
#include "MechaduinoController.h"

QLoggingCategory mechCat("MechaduinoCommunicator");

MechaduinoController::MechaduinoController(const QString &name, const QString &comPort, float step, QMap<int, int> *points, QObject *parent):
  QObject(parent),
  m_name(name),
  m_comPortName(comPort),
  m_step(step),
  m_points(points)
{
  m_port = new QSerialPort(comPort);
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

MechaduinoController::~MechaduinoController()
{
  if(m_port)
  {
    m_port->close();
    delete m_port;
  }
}

void MechaduinoController::changeFreq(qint64 newFreq)
{

}

float MechaduinoController::getPosition()
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

void MechaduinoController::setPosition(int newPosition)
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
