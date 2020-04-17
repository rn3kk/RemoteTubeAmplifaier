#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <QObject>
#include <QMap>
#include "OneFrequencyPoint.h"

class MainController :public QObject
{
  Q_OBJECT
public:
  MainController(const QString& pointsFilePath, QObject* parent = nullptr);    
  ~MainController();

Q_SIGNALS:

public Q_SLOTS:
  void newFrequency(int freq);
  void savePointToMap(const OneFrequencyPoint& point);
  void savePoints();

private:
   QMap<int, OneFrequencyPoint> m_points;
   QString m_pointsFilePath;
};

#endif // MAINCONTROLLER_H
