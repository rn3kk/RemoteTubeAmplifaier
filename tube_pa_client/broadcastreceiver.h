#ifndef BROADCASTRECEIVER_H
#define BROADCASTRECEIVER_H

#include <QObject>
#include <QUdpSocket>

class BroadcastReceiver : public QObject
{
  Q_OBJECT
public:
  explicit BroadcastReceiver(QObject *parent = nullptr);

public slots:
  void start();
  void readyRead();

signals:

private:
  QUdpSocket* m_socket = nullptr;

};

#endif // BROADCASTRECEIVER_H
