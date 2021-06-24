#ifndef PRESENTOR_H
#define PRESENTOR_H

#include <QObject>
#include <QWidget>
#include "../common/frontmodel.h"

class Presentor : public QObject
{
  Q_OBJECT
public:
  explicit Presentor(QString name, QString ip, QString port, QObject *parent = nullptr);
  ~Presentor();

signals:

private:
  QString m_name;
  class SocketWrapper* m_client;
  FrontModel m_model;
  QWidget* m_form;

};

#endif // PRESENTOR_H
