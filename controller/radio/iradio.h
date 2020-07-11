#ifndef IRADIO_H
#define IRADIO_H

#include <QObject>

class IRadio: public QObject //Interface for commutication with transceiver for receive freq from radio and set on radio new freq from this software
{
  Q_OBJECT
public:
  IRadio(QObject* parent = nullptr): QObject(parent){};

public slots:
  virtual void setTXFreq(int freq) = 0;

signals:
  void freqChanged(int freq);
  void radioConected();
  void radioDisconected();

};

#endif // IRADIO_H
