#ifndef APPLICAIONSETTINGS_H
#define APPLICAIONSETTINGS_H

#include <QVector>
#include <QString>
#include "../common/model.h"
class MechaduinoController;
class QSettings;

class ApplicaionSettings
{
public:
  enum RadioType
  {
    FLEX,
    YAESU,
    UNKNOWN_RADIO
  };

  ApplicaionSettings();
  ~ApplicaionSettings();
  static ApplicaionSettings& getInstance();
  bool loadSettings(const QString& configPath);

  QString getFlex6xxx_IP() const;
  int getFlex6xxx_port() const;

  void savePosition(QString name, QMap<int, int> *positions);

  RadioType getRadioType() const;
  QString getComPortName() const;
  QString getToken() const;
  QByteArray getTokenHash() const;

  quint16 getServerPort() const;

  QString getPaName() const;

public:
  MechaduinoController* m_mech1 = nullptr; //tune
  MechaduinoController* m_mech2 = nullptr; //load

private:
  QString getRandomString() const;

private:
  RadioType m_radioType;
  QString m_comPortName;//if yeasu
  QString m_flex6xxx_IP;//if flex
  int m_flex6xxx_port;//if flex

  quint16 m_serverPort;
  QByteArray m_initToken;

  QSettings* m_settings;

  QString m_paName = "P-140auto";


};

#endif // APPLICAIONSETTINGS_H
