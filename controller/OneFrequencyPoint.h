#ifndef ONEFREQUENCYPOINT_H
#define ONEFREQUENCYPOINT_H

#include <QMetaType>
#include<QString>

struct OneFrequencyPoint
{
  int freq;
  float tunePosition;
  float loadPosition;
  int picounturRelayBandPosition;

public:
  bool fromString(const QString& data);
  QString toString() const;
};

Q_DECLARE_METATYPE(OneFrequencyPoint);


#endif // ONEFREQUENCYPOINT_H
