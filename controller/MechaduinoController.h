#ifndef MECHADUINOCOMMUNICATOR_H
#define MECHADUINOCOMMUNICATOR_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QMutex>
#include <QSerialPort>
#include <QTimerEvent>
#include "../common/model.h"

class QSerialPort;

class MechaduinoController: public QObject
{
  Q_OBJECT
public:
  MechaduinoController(const QString& name, const QString& comPort, float step, QMap<int,int>* points, QObject* parent = nullptr);
  ~MechaduinoController();

  QString getName() const;
  int getLastPos() const;
  bool getManualMode() const;

signals:
  void changed();

public Q_SLOTS:
  void init();
  void changeFreq(int newFreq);
  void changeProperty(const Mechaduino& m);
  void savePosition();
  void tuneMode(bool mode);

private Q_SLOTS:
  void readyRead();
  void bytesWriten(qint64 bytes);
  void errorOccurred(QSerialPort::SerialPortError error);

private:
  void getPositionAsync();//read position in ReadyRead
  void timerEvent(QTimerEvent *event) override;
  //todo  наверное удалить
  void setPosition(qint64 newPosition);
  void setManualMode(bool setManualMode);

private:
  QSerialPort* m_port;
  QString m_name;
  QString m_comPortName;
  float m_step;
  QMap<int,int>* m_points;

  int m_lastFreq;
  int m_lastPos;
  bool m_tuneMode = false;
  bool m_manualMode = false;
  QByteArray m_readedPacket;

};

#endif // MECHADUINOCOMMUNICATOR_H
