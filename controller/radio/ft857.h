#ifndef FT857_H
#define FT857_H

#include <QSerialPort>
#include "iradio.h"

class ft857 : public IRadio
{
  Q_OBJECT
public:
  explicit ft857(const QString comPort, QObject* parent = nullptr);

public slots:
  void init();
  void setTXFreq(int freq) override;

private slots:
  void readyRead();
  void bytesWriten(qint64 bytes);

private:
  QSerialPort* m_port;
  QString m_comPortName;


};

#endif // FT857_H
