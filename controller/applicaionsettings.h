#ifndef APPLICAIONSETTINGS_H
#define APPLICAIONSETTINGS_H

#include <QVector>
#include <QString>
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

  QVector<MechaduinoController *> getMechConrollerList() const;
  void savePosition(QString name, QMap<int, int> *positions);

  RadioType getRadioType() const;

  QString getComPortName() const;

private:
  RadioType m_radioType;
  QString m_comPortName;//if yeasu
  QString m_flex6xxx_IP;//if flex
  int m_flex6xxx_port;//if flex

  QVector<MechaduinoController*> m_mechConrollerList;
  QSettings* m_settings;


};

#endif // APPLICAIONSETTINGS_H
