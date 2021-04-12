#ifndef PSMSERVER_H
#define PSMSERVER_H

#include <QObject>
#include <QAbstractSocket>
#include <QTcpServer>
#include <array>

class SocketWrapper;

class Server final: public QObject
{
  Q_OBJECT
public:
  explicit Server();
  ~Server();

signals:
  void sendToAllClients(const QByteArray& data);

public slots:
  void doWork();

protected slots:
  void socketData(const QByteArray& data);
  void newConnection();
  void socketDisconnected();

private:
  QTcpServer* m_server;
  QList<SocketWrapper*> m_socketList;

};

#endif // PSMSERVER_H
