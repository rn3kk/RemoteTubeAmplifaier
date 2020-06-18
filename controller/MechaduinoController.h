#ifndef MECHADUINOCOMMUNICATOR_H
#define MECHADUINOCOMMUNICATOR_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QMutex>

class QSerialPort;

class MechaduinoController: public QObject
{
  Q_OBJECT
public:
  MechaduinoController(const QString& name, const QString& comPort, float step, QMap<int,int>* points, QObject* parent = nullptr);
  ~MechaduinoController();

  QString getName() const;

signals:
  void changedPosition(int position);

public Q_SLOTS:
  void init();
  void changeFreq(int newFreq);
  void setPosition(qint64 newPosition);
  void savePosition();

private Q_SLOTS:
  void readyRead();
  void bytesWriten(qint64 bytes);

private:
  float getPosition();

private:
  QSerialPort* m_port;
  QString m_name;
  QString m_comPortName;
  float m_step;
  QMap<int,int>* m_points;

  int m_lastFreq;
  int m_lastPos;

};

#endif // MECHADUINOCOMMUNICATOR_H
