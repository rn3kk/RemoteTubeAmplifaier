#ifndef FRONTMODEL_H
#define FRONTMODEL_H

#include <QObject>
#include "model.h"

class FrontModel : public QObject, public Model
{
  Q_OBJECT
public:
  explicit FrontModel(QObject *parent = nullptr);

  void fromJson(const QByteArray& data);

signals:
  void modelChanged();

public slots:

};

#endif // FRONTMODEL_H
