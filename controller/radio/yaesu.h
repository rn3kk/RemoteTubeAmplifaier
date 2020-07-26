#ifndef FT857_H
#define FT857_H

#include <QSerialPort>
#include <QMutex>
#include "iradio.h"

class Yaesu : public IRadio
{
  Q_OBJECT  
public:
  explicit Yaesu(const QString comPort, QObject* parent = nullptr);

public slots:
  void init();
  void setTXFreq(int freq) override;

private:
  unsigned int reverse(unsigned int x);
  void timerEvent(QTimerEvent *event) override;

private:
  QSerialPort* m_port;
  QString m_comPortName;
  QMutex m_writeToSerialPortMutex;

};

#endif // FT857_H
