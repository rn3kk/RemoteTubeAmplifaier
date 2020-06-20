#ifndef APPLICAIONSETTINGS_H
#define APPLICAIONSETTINGS_H

#include <QVector>
#include <QString>
class MechaduinoController;
class QSettings;

class ApplicaionSettings
{
public:
  ApplicaionSettings();
  ~ApplicaionSettings();
  static ApplicaionSettings& getInstance();
  bool loadSettings(const QString& configPath);

  QString getFlex6xxx_IP() const;
  int getFlex6xxx_port() const;

  QVector<MechaduinoController *> getMechConrollerList() const;
  void savePosition(QString name, QMap<int, int> *positions);

private:
  QString m_flex6xxx_IP;
  int m_flex6xxx_port;
  QVector<MechaduinoController*> m_mechConrollerList;
  QSettings* m_settings;
};

#endif // APPLICAIONSETTINGS_H
