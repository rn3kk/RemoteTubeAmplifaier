#include <QFile>
#include <QLoggingCategory>
#include "OneFrequencyPoint.h"
#include "MainController.h"
QLoggingCategory contCat("MainController");

MainController::MainController(const QString &pointsFilePath, QObject *parent):
  QObject(parent),
  m_pointsFilePath(pointsFilePath)
{
  QFile file(pointsFilePath);
  if(!file.open(QIODevice::ReadOnly))
  {
    qCCritical(contCat) << "Can't open file with frequency points";
  }
  else
  {
    bool first = true;
    while(!file.atEnd())
    {
      if(first)
      {
        first = false;
        continue;
      }
      QString str = file.readLine();
      OneFrequencyPoint point;
      if(point.fromString(str))
      {
        m_points.insert(point.freq, point);
      }
    }

  }

}

void MainController::newFrequency(int freq)
{

}
