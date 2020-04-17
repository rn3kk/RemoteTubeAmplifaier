#include <QFile>
#include <QMapIterator>
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

MainController::~MainController()
{

}

void MainController::newFrequency(int freq)
{

}

void MainController::savePointToMap(const OneFrequencyPoint &point)
{
  m_points.remove(point.freq);
  m_points.insert(point.freq, point);
}

void MainController::savePoints()
{
  QFile file(m_pointsFilePath);
  if(!file.open(QIODevice::WriteOnly | QFile::Text))
  {
    qCCritical(contCat) << "Can't open file with frequency points for save points";
    return;
  }

  for(QMap<int, OneFrequencyPoint>::const_iterator i= m_points.constBegin(); i != m_points.end(); ++i)
  {
    file.write(i.value().toString().toStdString().c_str());
  }
  file.close();
}
