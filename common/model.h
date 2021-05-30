#ifndef MODEL_H
#define MODEL_H

#include <QMap>
#include <QMutex>
#include <QString>
#include <QByteArray>

class Model
{
public:
  Model();

  virtual ~Model(){};

protected:
  void markChanged();

protected:
  bool m_power;  //on off
  QMap<QString, int> m_mechaduinoStates; //negative degrees - mech not connected
  int m_radioFreq; //if empty or degreese - radio not connected
  int m_relayNumber; // 0 - empty
  bool m_tuneMode = false;
  bool m_extenalProtection = false;
  bool m_isChanged = false;
  QMutex m_mutex;
};

#endif // MODEL_H
