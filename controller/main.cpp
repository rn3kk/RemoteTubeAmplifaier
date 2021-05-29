#include <QCoreApplication>
#include <QVector>
#include <QThread>
#include <server.h>
#include "statemodel.h"
#include "radio/radiofactory.h"
#include "radio/iradio.h"
#include "MechaduinoController.h"
#include "applicaionsettings.h"
#include "broadcastinformer.h"
#include "../common/model.h"

int main(int argc, char *argv[])
{
  qRegisterMetaType<QPair<QString, QString>>("QPair<QString, QString>");
  QCoreApplication a(argc, argv);

  QString configPath = "/home/user/projects/RemoteTubeAmplifaier/controller/config.conf";
  //QString configPath = "config.conf";
  ApplicaionSettings& setting = ApplicaionSettings::getInstance();
  if(!setting.loadSettings(configPath))
    exit(0);

  Model m;

  BroadcastInformer bi;
  QThread th;
  bi.moveToThread(&th);
  QObject::connect(&th, &QThread::started, &bi, &BroadcastInformer::start, Qt::QueuedConnection);
  th.start();

  StateModel& model = StateModel::getInstance();

  Server server;
  QThread serverThread;
  server.moveToThread(&serverThread);
  QObject::connect(&serverThread, &QThread::started, &server, &Server::doWork, Qt::QueuedConnection);
  serverThread.start();

  QObject::connect(&model, &StateModel::modelChanged, &server, &Server::sendToAllClients);
  QObject::connect(&server, &Server::changeModel, &model,  &StateModel::needChange);

  
  IRadio* radio = RadioFactory::getRadio();
  QObject::connect(radio, &IRadio::freqChanged, &model, &StateModel::setRadioFreq);

  const QVector<MechaduinoController*>& v = setting.getMechConrollerList();
  QVector<MechaduinoController*>::const_iterator i;
  for(i = v.begin(); i!= v.end(); ++i)
  {
    QObject::connect(*i, &MechaduinoController::changedPosition ,&model, &StateModel::needChange);
    QObject::connect(radio, &IRadio::freqChanged, *i, &MechaduinoController::changeFreq, Qt::QueuedConnection);
    QObject::connect(&model, &StateModel::tuneMode, *i, &MechaduinoController::tuneMode);
  }

  return a.exec();
}

