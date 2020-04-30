#ifndef MECHADUINOCOMMUNICATOR_H
#define MECHADUINOCOMMUNICATOR_H

#include <QObject>
#include <QString>
#include <QMap>

class QSerialPort;

class MechaduinoController: public QObject
{
  Q_OBJECT
public:
  MechaduinoController(const QString& name, const QString& comPort, float step, QMap<int,int>* points, QObject* parent = nullptr);
  ~MechaduinoController();

public Q_SLOTS:
  static void changeFreq(qint64 newFreq);

private:
  float getPosition();
  void setPosition(int newPosition);

private:
  QSerialPort* m_port;
  QString m_name;
  QString m_comPortName;
  float m_step;
  QMap<int,int>* m_points;
};

#endif // MECHADUINOCOMMUNICATOR_H
