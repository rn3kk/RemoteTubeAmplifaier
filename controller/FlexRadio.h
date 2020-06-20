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
  void setTXFreq(int freq);
  void isConnected();
  void isDisconected();
  void readData();
  void socketChangeState(QAbstractSocket::SocketState socketState);
  void socketError(QAbstractSocket::SocketError error);
  void writen(qint64 bytes);

Q_SIGNALS:
  void changeTXFreq(int freq);

private:
  void timerEvent(QTimerEvent *event) override;
  void parseVfomSLICE(const QByteArray& data);

private:
  const QString m_radioHost;
  quint16 m_radioPort;
  QTcpSocket* m_socket;

  QByteArray m_remoteVersionProtocol;
  QByteArray m_handleSeq;

  float m_slice1Freq;
  float m_slice2Freq;
  bool m_activeSlice1;
  bool m_activeSlice2;
  bool m_A_isTX=false;
  bool m_B_isTX=false;
};

#endif // RADIO_H
