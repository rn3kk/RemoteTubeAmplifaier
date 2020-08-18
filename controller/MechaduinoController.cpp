#include <QApplication>
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
  QObject::connect(m_port, &QSerialPort::errorOccurred, this, &MechaduinoController::errorOccurred, Qt::QueuedConnection);
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

void MechaduinoController::getPositionAsync() //read position in ReadyRead
{
  qDebug() << "getPositionAsync()";
  if(m_port->isOpen())
  {
    m_port->write("p\n");
    //m_port->flush();
    if(!m_port->waitForBytesWritten(500))
      return;
    bool packetIsReaded(false);
    while (!packetIsReaded)
    {
      if(m_port->waitForReadyRead(250))
      {
        m_readedPacket.append(m_port->readAll());
        if(m_readedPacket.contains("\r\n"))
          packetIsReaded = true;
      }
      else
        break;
    }
    if(packetIsReaded)
    {
      int pos = m_readedPacket.indexOf("Angle: ");
      pos+= strlen("Angle: ");
      QString angleStr;
      for(int i=0; i< 5; ++i)
      {
        char c = m_readedPacket.at(pos+i);
        if(c !='.')
        {
          angleStr.append(c);
        }
        else
          break;
      }
      qDebug() << "Read packet:" << m_readedPacket;
      qDebug() << "Angle is:" << angleStr;
      m_lastPos=angleStr.toInt();
      Q_EMIT changedPosition(m_lastPos);
    }
    m_readedPacket.clear();
  }
}

void MechaduinoController::timerEvent(QTimerEvent *event)
{
  killTimer(event->timerId());
  if(m_manualMode)
     getPositionAsync(); //read position in ReadyRead
  startTimer(250);
}

QString MechaduinoController::getName() const
{
  return m_name;
}

void MechaduinoController::setPosition(qint64 newPosition)
{
  qDebug() << "setPosition(" << newPosition<<")";
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
    savePosition();
    ApplicaionSettings::getInstance().savePosition(m_name, m_points);
    m_manualMode = false;
  }
  m_tuneMode = mode;
}

void MechaduinoController::manualMode(bool manualMode)
{
  if(!m_tuneMode)
    return;
  if(m_manualMode && manualMode == false) // выключается ручной режим настройки
  {
    if(m_port && m_port->isOpen())
    {
      m_port->write("xy");
      m_port->flush();
      setPosition(m_lastPos);
    }
  }
  if(manualMode)
    startTimer(500);
  m_manualMode = manualMode;

}

void MechaduinoController::readyRead()
{
  if(!m_port || !m_port->isOpen())
  {
    return;
  }

  qint64 b = m_port->bytesAvailable();
  QByteArray a;
  a.resize(b);
  b = m_port->read(a.data(), b);
  if(a.length()>0)
    qDebug() << "ReadFrom Mechaduino COM port" << a;
}

void MechaduinoController::bytesWriten(qint64 bytes)
{
  QDateTime d = QDateTime::currentDateTime();
  qDebug() << "Writen " << bytes << " " << d;
}

void MechaduinoController::errorOccurred(QSerialPort::SerialPortError error)
{
  qDebug()<< "Serial port error " << error;
  if(error == 0)
    return;

}
