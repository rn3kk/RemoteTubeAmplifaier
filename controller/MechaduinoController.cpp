#include <QSerialPort>
#include <QThread>
#include <QLoggingCategory>
#include <QMap>
#include "MechaduinoController.h"

QLoggingCategory mechCat("MechaduinoCommunicator");

MechaduinoController::MechaduinoController(const QString &name, const QString &comPort, float step, QMap<int, int> *points, QObject *parent):
  QObject(parent),
  m_name(name),
  m_comPortName(comPort),
  m_step(step),
  m_points(points)
{
//  QThread* thread = new QThread();
//  this->moveToThread(thread);
//  QObject::connect(thread, &QThread::started, this, &MechaduinoController::init, Qt::QueuedConnection);
//  thread->start();

  m_port = new QSerialPort(m_comPortName);
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

void MechaduinoController::init()
{

}

int pos;
int begin;
int end;
void MechaduinoController::changeFreq(int newFreq)
{
  begin = newFreq/100; // отбрасываем последние два числа
  begin *= 100;
  end = newFreq % 100; //остаток от деления есть два последних числа частоты
  if(end >= 0 && end < 25)
    pos = m_points->key(begin); //+00
  else if(end >= 25 && end < 50)
    pos = m_points->key(begin+25); //+25
  else if(end >= 50 && end < 75)
    pos = m_points->key(begin+50); //+50
  else if(end >= 75 && end <100)
    pos = m_points->key(begin+75); //+75

  float f = getPosition();
  setPosition(pos);
}

float MechaduinoController::getPosition()
{
  if(m_port->isOpen())
  {
    bool b = m_port->waitForReadyRead(100);
    if(b)
      m_port->readAll();
    m_port->write("p");
    m_port->waitForBytesWritten();
    m_port->waitForReadyRead();
    QString position = m_port->readAll();
    //20.05\r\nEnter setpoint:\r\n90.00\r\nstepNumber: 0 , Angle: 59.72, raw encoder: 2722\r\n
    //qDebug(mechCat) << position;
    int f = position.indexOf("Angle: ");
    QString posAngl= position.mid(f+7, 5);

    m_port->write("xy");
    m_port->waitForBytesWritten();

    return position.toFloat();
  }
  return -1;
}

void MechaduinoController::setPosition(int newPosition)
{
  if(newPosition < 0 || newPosition > 360)
    return;

  if(m_port && m_port->isOpen())
  {
    bool b = m_port->waitForReadyRead(100);
    if(b)
      m_port->readAll();
    m_port->write("r");
    m_port->write(QString::number(newPosition).toStdString().c_str());
    m_port->waitForBytesWritten();
    m_port->waitForReadyRead();
    QString position = m_port->readAll();
    qDebug(mechCat) << position;
  }
  //thread()->msleep(100);
}
