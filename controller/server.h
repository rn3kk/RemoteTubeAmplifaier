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
  explicit Server(quint16 port);
  ~Server();

signals:

public slots:
  void doWork();

protected slots:
  void socketData(const QByteArray& data);
  void newConnection();    

private slots:
  void timerEvent(QTimerEvent *event) override;

private:
  quint16 m_listeningPort;
  QTcpServer* m_server;
  SocketWrapper* m_socket;

};

#endif // PSMSERVER_H
