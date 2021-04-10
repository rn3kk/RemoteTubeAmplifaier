#ifndef AMPINFO_H
#define AMPINFO_H

#include <QString>

class AmpInfo
{
public:
  AmpInfo();
  AmpInfo(QString name, QString ip);
  AmpInfo(QByteArray xml);
  AmpInfo(AmpInfo const &ai);
  ~AmpInfo();

  QByteArray toXml() const;
  bool isValid();

  QString paName() const;
  QString paIp() const;
  quint16 paPort() const;
  QString timeCreating() const;

private:
  QString m_paName;
  QString m_paIp;
  quint16 m_paPort;
  QString m_timeCreating;
};

//Q_DECLARE_METATYPE(AmpInfo)

#endif // AMPINFO_H
