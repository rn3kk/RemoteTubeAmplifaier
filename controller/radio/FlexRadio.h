#ifndef RADIO_H
#define RADIO_H

#include <QTcpSocket>
#include <QString>
#include "iradio.h"

class FlexRadio: public IRadio
{
  Q_OBJECT
public:
  explicit FlexRadio(const QString& host, quint16 port, QObject* parent = nullptr);

public Q_SLOTS:
  void setTXFreq(int freq) override;
  void isConnected();
  void isDisconected();
  void readData();
  void socketChangeState(QAbstractSocket::SocketState socketState);
  void socketError(QAbstractSocket::SocketError error);
  void writen(qint64 bytes);

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
