#include <QFile>
#include <QMapIterator>
#include <QLoggingCategory>
#include "applicaionsettings.h"
#include "OneFrequencyPoint.h"
#include "MainController.h"
QLoggingCategory contCat("MainController");

MainController::MainController(const QString &pointsFilePath, QObject *parent):
  QObject(parent),
  m_pointsFilePath(pointsFilePath)
{
  m_points = ApplicaionSettings::getInstance().getPoints();
}

MainController::~MainController()
{

}

void MainController::newFrequency(int freq)
{
  if(m_points.contains(freq))
  {
    OneFrequencyPoint p = m_points.value(freq);
    Q_EMIT newPoint(p);
  }
}

void MainController::savePointToMap(const OneFrequencyPoint &point)
{
  m_points.remove(point.freq);
  m_points.insert(point.freq, point);
}

void MainController::savePoints()
{

}
