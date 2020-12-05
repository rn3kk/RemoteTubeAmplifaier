#ifndef SOCKETWRAPPER_H
#define SOCKETWRAPPER_H

#include <QObject>
#include <QTcpSocket>

class SocketWrapper: public QObject
{
  Q_OBJECT
public:
  SocketWrapper(QObject* parent = nullptr);
  virtual ~SocketWrapper();

signals:
  void avaliableData(const QByteArray& data);
  void socketConnectionStatus(bool connectionStatus);

public slots:
  void writeToSocket(const QByteArray& data);

protected slots:
  void readyRead();
  virtual void stateChanged(QAbstractSocket::SocketState socketState){}
  void connected();
  void disconnected();

protected:
  void connectSocketSignalsToSlots();

protected:
  QTcpSocket* m_socket;
  QString m_name;

};

#endif // SOCKETWRAPPER_H
