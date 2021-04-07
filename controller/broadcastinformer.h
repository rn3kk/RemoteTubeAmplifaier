#ifndef BROADCASTINFORMER_H
#define BROADCASTINFORMER_H

#include <QUdpSocket>
#include <QObject>

class BroadcastInformer: public QObject
{
  Q_OBJECT
public:
  BroadcastInformer(QObject* parent = nullptr);
  ~BroadcastInformer();

public slots:
  void start();

private:
  void timerEvent(QTimerEvent *event) override;
  QString getIp() const;
  QUdpSocket* m_socket = nullptr;

};

#endif // BROADCASTINFORMER_H
