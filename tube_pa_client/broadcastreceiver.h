#ifndef BROADCASTRECEIVER_H
#define BROADCASTRECEIVER_H

#include <QObject>
#include <QUdpSocket>
#include "../common/ampinfo.h"



class BroadcastReceiver : public QObject
{
  Q_OBJECT
public:
  explicit BroadcastReceiver(QObject *parent = nullptr);

public slots:
  void start();
  void readyRead();

signals:
  void findAmplifaier(AmpInfo ai);

private:
  QUdpSocket* m_socket = nullptr;

};

#endif // BROADCASTRECEIVER_H
