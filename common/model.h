#ifndef MODEL_H
#define MODEL_H

#include <QMap>
#include <QMutex>
#include <QString>
#include <QByteArray>

struct Mechaduino
{
  QString name;
  int position;
  bool manualMode;

  Mechaduino(const Mechaduino& m);
  Mechaduino();

  QString toString() const ;
  bool fromString(QString str);
  bool isValid();
};

class Model
{
public:
  Model();
  virtual ~Model(){};

  bool power() const;
  int radioFreq() const;
  int relayNumber() const;
  bool tuneMode() const;
  bool extenalProtection() const;
  QList<Mechaduino> mechaduinos() const;

protected:
  void markChanged();

protected:
  int m_power;  //on off
  QList<Mechaduino> m_mechaduinos;
  int m_mech1_pos, m_mech2_pos;

  int m_radioFreq; //if empty or degreese - radio not connected
  int m_relayNumber; // 0 - empty
  int m_tuneMode = 0;
  int m_extenalProtection = 0;
  int m_isChanged = 0;
  QMutex m_mutex;
};

#endif // MODEL_H
