#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <QObject>
#include <QMap>

struct OneFrequencyPoint
{
  int freq;
  int capPosition;
  int loadPosition;
  int picounturRelayBandPosition;

  bool fromString(const QString& data);
  QString toString();
};

class MainController :public QObject
{
  Q_OBJECT
public:
  MainController(const QString& pointsFilePath, QObject* parent = nullptr);

Q_SIGNALS:

public Q_SLOTS:
  void newFrequency(int freq);

private:
   QMap<int, OneFrequencyPoint> m_points;
   QString m_pointsFilePath;
};

#endif // MAINCONTROLLER_H
