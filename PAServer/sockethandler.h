#ifndef SOCKET_H
#define SOCKET_H

#include <QObject>
#include <QTcpSocket>
#include "typetokenparser.h"

class SocketHandler final : public QObject, public TypeToken
{
  Q_OBJECT
public:
  SocketHandler();
  SocketHandler(QTcpSocket *socket, QObject* parent = nullptr);
  SocketHandler(const SocketHandler &handler);
  ~SocketHandler() override;
  void destroy();

  QString ip() const;

private:
  void timerEvent(QTimerEvent *event) override;

public slots:
  void doInit();
  void readyRead();
  void disconnected();
  void writeData(const QByteArray& data);

signals:
  void avaliableData(const QByteArray& data);
  void finished();

private:
   QString m_ip;
   QTcpSocket* m_socket;
};

Q_DECLARE_METATYPE(SocketHandler);

#endif // SOCKET_H
