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

int main(int argc, char *argv[])
{
  qRegisterMetaType<QPair<QString, QString>>("QPair<QString, QString>");
  QCoreApplication a(argc, argv);

//  Form f;
//  f.show();

//  Vfo* vfoPanel = new Vfo();
//  QObject::connect(&f, &Form::tuneMode, vfoPanel, &Vfo::tuneMode, Qt::QueuedConnection);
//  QHBoxLayout* mechaduinoContainer = f.findChild<QHBoxLayout*>("mechaduinoContainer");
//  mechaduinoContainer->addWidget(vfoPanel);

  QString configPath = "/home/user/projects/RemoteTubeAmplifaier/controller/config.conf";
  //QString configPath = "config.conf";
  ApplicaionSettings& setting = ApplicaionSettings::getInstance();
  if(!setting.loadSettings(configPath))
    exit(0);

  BroadcastInformer bi;
  QThread th;
  bi.moveToThread(&th);
  QObject::connect(&th, &QThread::started, &bi, &BroadcastInformer::start, Qt::QueuedConnection);
  th.start();

  StateModel& model = StateModel::getInstance();
  model.fromJson(model.toJson());

  Server server;
  QThread serverThread;
  server.moveToThread(&serverThread);
  QObject::connect(&serverThread, &QThread::started, &server, &Server::doWork, Qt::QueuedConnection);
  serverThread.start();

  QObject::connect(&model, &StateModel::modelChanged, &server, &Server::sendToAllClients);
  QObject::connect(&server, &Server::changeModel, &model,  &StateModel::needChange);

  
  IRadio* radio = RadioFactory::getRadio();
  QObject::connect(radio, &IRadio::freqChanged, &model, &StateModel::setRadioFreq);

//  QObject::connect(&f, &Form::setFreq, radio, &IRadio::setTXFreq, Qt::QueuedConnection);//TODO debug
//  QObject::connect(vfoPanel, &Vfo::tuneVFOTxToNewFreq, radio, &IRadio::setTXFreq, Qt::QueuedConnection);//TODO debug

//  const QVector<MechaduinoController*>& v = setting.getMechConrollerList();
//  QVector<MechaduinoController*>::const_iterator i;
//  for(i = v.begin(); i!= v.end(); ++i)
//  {
//    MechPanel* mp = new MechPanel((*i)->getName());
//    mechaduinoContainer->addWidget(mp);
//    QObject::connect(mp, &MechPanel::changePosition, *i, &MechaduinoController::setPosition, Qt::QueuedConnection);
//    QObject::connect(*i, &MechaduinoController::changedPosition ,mp, &MechPanel::newPosition, Qt::QueuedConnection);
//    QObject::connect(radio, &IRadio::freqChanged, *i, &MechaduinoController::changeFreq, Qt::QueuedConnection);
//    QObject::connect(vfoPanel, &Vfo::savePosition, *i, &MechaduinoController::savePosition, Qt::QueuedConnection);
//    QObject::connect(&f, &Form::tuneMode, *i, &MechaduinoController::tuneMode, Qt::QueuedConnection);
//    QObject::connect(&f, &Form::tuneMode, mp, &MechPanel::tuneMode, Qt::QueuedConnection);
//    QObject::connect(mp, &MechPanel::manualMode, *i, &MechaduinoController::manualMode, Qt::QueuedConnection);
//  }

  return a.exec();
}

