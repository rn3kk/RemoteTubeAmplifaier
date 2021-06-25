#ifndef BACKMODEL_H
#define BACKMODEL_H

#include "model.h"
#include <QObject>

class BackModel : public QObject, public Model
{
  Q_OBJECT
public:  
  static BackModel& getInstance();
  void setMechaduinos(QMap<QString, int> mechaduinos);
  void setPwr(bool pwr);
  QByteArray toJson();

signals:
  void modelIsChanged(const QByteArray& data);

public slots:
  void externalProtection(bool state);
  void change(QPair<QString, QString> pair);
  void setRadioFreq(int radioFreq);

private slots:
  void timerEvent(QTimerEvent *event) override;

private:
  explicit BackModel(QObject *parent = nullptr);

};

#endif // BACKMODEL_H
