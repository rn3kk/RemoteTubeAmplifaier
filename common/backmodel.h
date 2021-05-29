#ifndef BACKMODEL_H
#define BACKMODEL_H

#include "model.h"
#include <QObject>

class BackModel : public QObject, public Model
{
  Q_OBJECT
public:  
  BackModel& getInstance();
  void setMechaduinos(QMap<QString, int> mechaduinos);

  QByteArray toJson();

signals:
  void modelIsChanged();

public slots:
  void externalProtection(bool state);
  void changeModel(QPair<QString, QString> pair);

private slots:
  void timerEvent(QTimerEvent *event) override;

private:
  explicit BackModel(QObject *parent = nullptr);
  void markChanged();

};

#endif // BACKMODEL_H
