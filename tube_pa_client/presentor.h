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

public slots:
  void modelChanged();

private slots:
  void needChangePWR(bool pwr);

private:
  QString m_name;
  FrontModel m_model;
  class SocketWrapper* m_client;
  class Form * m_form;
};

#endif // PRESENTOR_H
