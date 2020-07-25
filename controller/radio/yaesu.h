#ifndef FT857_H
#define FT857_H

#include <QSerialPort>
#include "iradio.h"

class Yaesu : public IRadio
{
  Q_OBJECT
public:
  explicit Yaesu(const QString comPort, QObject* parent = nullptr);

public slots:
  void init();
  void setTXFreq(int freq) override;

private slots:
  void readyRead();
  void bytesWriten(qint64 bytes);  

private:
  void timerEvent(QTimerEvent *event) override;

private:
  QSerialPort* m_port;
  QString m_comPortName;


};

#endif // FT857_H
