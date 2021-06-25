#ifndef FRONTMODEL_H
#define FRONTMODEL_H

#include <QObject>
#include "model.h"

class FrontModel : public QObject, public Model
{
  Q_OBJECT
public:
  explicit FrontModel(QObject *parent = nullptr);


signals:
  void modelChanged();

public slots:
  void fromJson(const QByteArray& data);

};

#endif // FRONTMODEL_H
