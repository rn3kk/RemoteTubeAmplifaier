#include <QDebug>
#include <QStringList>
#include "OneFrequencyPoint.h"

//one point
//freq   tune(0..30)  load(0..360)  RelayPos(0..11)
//710000 120   350   2
bool OneFrequencyPoint::fromString(const QString &data)
{
  if(data.isNull() || data.isEmpty())
    return false;
  QStringList list;
  list = data.split(" ");
  if(list.length() != 4)
  {
    qDebug()<<"String not have full data:" << data;
    return false;
  }
  freq = list.at(0).toInt();
  tunePosition = list.at(1).toFloat();
  loadPosition = list.at(2).toFloat();
  picounturRelayBandPosition = list.at(3).toInt();
  return true;
}

QString OneFrequencyPoint::toString() const
{
  QString str;
  str = QString::number(freq) + " " +
      QString::number(tunePosition) + " " +
      QString::number(loadPosition) + " " +
      QString::number(picounturRelayBandPosition);
  return str;
}
