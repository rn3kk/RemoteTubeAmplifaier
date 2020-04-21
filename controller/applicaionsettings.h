#ifndef APPLICAIONSETTINGS_H
#define APPLICAIONSETTINGS_H

#include <QString>
#include <QMap>
#include <OneFrequencyPoint.h>

class ApplicaionSettings
{
public:
  ApplicaionSettings();
  static ApplicaionSettings& getInstance();
  bool loadSettings(const QString& configPath);

  QString getFlex6xxx_IP() const;
  int getFlex6xxx_port() const;
  QMap<int, OneFrequencyPoint> getPoints() const;

private:
  QString m_flex6xxx_IP;
  int m_flex6xxx_port;
  QMap<int, OneFrequencyPoint> m_points;
};

#endif // APPLICAIONSETTINGS_H
