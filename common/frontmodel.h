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

private:
  QByteArray m_data_queue;

};

#endif // FRONTMODEL_H
