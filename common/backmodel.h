#ifndef BACKMODEL_H
#define BACKMODEL_H

#include "model.h"
#include <QObject>

class BackModel : public QObject, public Model
{
  Q_OBJECT
public:  
  static BackModel& getInstance();
  void addMechaduinos(Mechaduino mechaduino);
  void setPwr(bool pwr);
  void setTuneMode(bool tm);
  void setRelay(int relay);
  QByteArray toJson();
  void changeMechaduino(const Mechaduino &mechaduino);

signals:
  void modelIsChanged(const QByteArray& data);

public slots:
  void externalProtection(bool state);
  void setRadioFreq(int radioFreq);

private slots:
  void timerEvent(QTimerEvent *event) override;

private:
  explicit BackModel(QObject *parent = nullptr);

};

#endif // BACKMODEL_H
