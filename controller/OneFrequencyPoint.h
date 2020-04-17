#ifndef ONEFREQUENCYPOINT_H
#define ONEFREQUENCYPOINT_H

#include<QString>

struct OneFrequencyPoint
{
  int freq;
  float tunePosition;
  float loadPosition;
  int picounturRelayBandPosition;

  bool fromString(const QString& data);
  QString toString();
};

#endif // ONEFREQUENCYPOINT_H
