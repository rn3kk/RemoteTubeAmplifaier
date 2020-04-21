#ifndef MECHADUINOCOMMUNICATOR_H
#define MECHADUINOCOMMUNICATOR_H

#include <QObject>
#include <QString>

class QSerialPort;

class MechaduinoCommunicator: public QObject
{
  Q_OBJECT
public:
  MechaduinoCommunicator(const QString& comPortName, QObject* parent = nullptr);
  ~MechaduinoCommunicator();

  float getPosition();
  void setPosition(int newPosition);

private:
  QSerialPort* m_port;
  QString m_comPortName;
};

#endif // MECHADUINOCOMMUNICATOR_H
