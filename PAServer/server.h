#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include "sockethandler.h"

class Server final: public QObject
{
  Q_OBJECT
public:
  Server(QObject* parent = nullptr);

protected slots:
  void newConnection();

signals:
  void newSocketHandler(SocketHandler * handler);

private:
  QTcpServer* m_server;

};

#endif // SERVER_H
