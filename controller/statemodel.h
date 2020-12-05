#ifndef STATEMODEL_H
#define STATEMODEL_H

#include <QObject>
#include <QMap>
#include <QMutex>

class StateModel: public QObject
{
  Q_OBJECT
public:
  StateModel(QObject* parent = nullptr);

private:
  void markChanged();

public slots:
  void setPower(bool power);
  void mechaduinoPosition(const QString& name, int pos);

private slots:
  void timerEvent(QTimerEvent *event) override;

signals:
  void modelChanged(const QByteArray& model);

private:
  bool m_power;  //on off
  QMap<QString, int> m_mechaduinoStates; //negative degrees - mech not connected
  QString m_radioFreq; //if empty or degreese - radio not connected
  int m_relayPinNumber; // 0 - empty
  bool m_isChanged;
  QMutex m_mutex;

};

#endif // STATEMODEL_H
