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

public slots:
  void doInit();

protected slots:
  void newConnection();

private:
  QTcpServer* m_server;

};

#endif // SERVER_H
