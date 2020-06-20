#include <QApplication>
#include <QSerialPort>
#include <QDateTime>
#include <QThread>
#include <QLoggingCategory>
#include <QMap>
#include "applicaionsettings.h"
#include "MechaduinoController.h"

QLoggingCategory mechCat("MechaduinoCommunicator");

MechaduinoController::MechaduinoController(const QString &name, const QString &comPort, float step, QMap<int, int> *points, QObject *parent):
  QObject(parent),
  m_name(name),
  m_comPortName(comPort),
  m_step(step),
  m_points(points)
{
  QThread* thread = new QThread();
  this->moveToThread(thread);
  QObject::connect(thread, &QThread::started, this, &MechaduinoController::init, Qt::QueuedConnection);
  thread->start();
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
  m_port = new QSerialPort(m_comPortName);
  m_port->setDataBits(QSerialPort::Data8);
  m_port->setParity(QSerialPort::OddParity);
  m_port->setFlowControl(QSerialPort::SoftwareControl);
  m_port->setStopBits(QSerialPort::OneStop);
  m_port->setBaudRate(QSerialPort::Baud9600);
  QObject::connect(m_port, &QSerialPort::readyRead, this, &MechaduinoController::readyRead, Qt::QueuedConnection);
  if(!m_port->open(QSerialPort::ReadWrite))
  {
    qCritical(mechCat) << "connection open error: "
                       << m_port->errorString();
    delete m_port;
    m_port = nullptr;
  }
  else
  {
    m_port->write("xy");
  }
}

void MechaduinoController::changeFreq(int newFreq)
{  
  int freq;
  int begin = newFreq/100; // отбрасываем последние два числа
  begin *= 100;
  int end = newFreq % 100; //остаток от деления есть два последних числа частоты
  if(end >= 0 && end < 25)
    freq = begin;
  else if(end >= 25 && end < 50)
    freq = begin + 25;
  else if(end >= 50 && end < 75)
    freq = begin + 50;
  else if(end >= 75 && end <100)
    freq = begin + 75;

  m_lastFreq = freq;
  int pos = m_points->value(m_lastFreq); //+00

  qDebug() << "Change freq " << m_lastFreq << " " << pos;
   setPosition(pos);
}

float MechaduinoController::getPosition()
{
  if(m_port->isOpen())
  {
    m_port->write("p\n");
    m_port->waitForBytesWritten();

    QByteArray requestData = m_port->readAll();
    while (m_port->waitForReadyRead(10))
      requestData += m_port->readAll();

    m_port->write("xy");

    //20.05\r\nEnter setpoint:\r\n90.00\r\nstepNumber: 0 , Angle: 59.72, raw encoder: 2722\r\n
    //qDebug(mechCat) << position;
    int f = requestData.indexOf("Angle: ");
    QString posAngl= requestData.mid(f+7, 5);
    return requestData.toFloat();
  }
  return -1;
}

QString MechaduinoController::getName() const
{
  return m_name;
}

void MechaduinoController::setPosition(qint64 newPosition)
{
  if(m_lastPos == newPosition)
    return;
  if(!m_port || !m_port->isOpen())
  {
    return;
  }
  if(newPosition >= 0 && newPosition <= 360)
  {    
    QString  str = "r"+QString::number(newPosition) + "\n";
    m_port->write(str.toStdString().c_str());
    m_port->flush();
    m_lastPos = newPosition;
    Q_EMIT changedPosition(newPosition);
  }
}

void MechaduinoController::savePosition()
{
  qDebug() << "Save position " << m_lastFreq << " " << m_lastPos;
  if(m_tuneMode)
    m_points->insert(m_lastFreq, m_lastPos);
}

void MechaduinoController::tuneMode(bool mode)
{
  if(m_tuneMode && mode==false) //выключается tune mode
  {
    ApplicaionSettings::getInstance().savePosition(m_name, m_points);
  }
  m_tuneMode = mode;
}

void MechaduinoController::readyRead()
{
  qint64 b = m_port->bytesAvailable();
  QByteArray a;
  a.resize(b);
  b = m_port->read(a.data(), b);
}

void MechaduinoController::bytesWriten(qint64 bytes)
{
  QDateTime d = QDateTime::currentDateTime();
  qDebug() << "Writen " << bytes << " " << d;
}
