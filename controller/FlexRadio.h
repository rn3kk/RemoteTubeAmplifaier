#ifndef RADIO_H
#define RADIO_H

#include <QObject>
#include <QTcpSocket>
#include <QString>

class FlexRadio: public QObject
{
  Q_OBJECT
public:
  explicit FlexRadio(const QString& host, quint16 port, QObject* parent = nullptr);

public Q_SLOTS:
  void isConnected();
  void isDisconected();
  void readData();
  void socketChangeState(QAbstractSocket::SocketState socketState);
  void socketError(QAbstractSocket::SocketError error);
  void writen(qint64 bytes);

Q_SIGNALS:
  void radioFrequency(quint64 freq);

private:
  void timerEvent(QTimerEvent *event) override;
  void parseVfomSLICE(const QByteArray& data);

private:
  const QString m_radioHost;
  quint16 m_radioPort;
  QTcpSocket* m_socket;

  QByteArray m_remoteVersionProtocol;
  QByteArray m_handleSeq;

};

#endif // RADIO_H
