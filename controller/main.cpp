#include <QApplication>
#include <QThread>
#include <QVector>
#include <QHBoxLayout>
#include <QLabel>
#include "server.h"
#include "vfo.h"
#include "form.h"
#include "mechpanel.h"
#include "statemodel.h"
#include "radio/radiofactory.h"
#include "radio/iradio.h"
#include "MechaduinoController.h"
#include "applicaionsettings.h"

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

//  Form f;
//  f.show();

//  Vfo* vfoPanel = new Vfo();
//  QObject::connect(&f, &Form::tuneMode, vfoPanel, &Vfo::tuneMode, Qt::QueuedConnection);
//  QHBoxLayout* mechaduinoContainer = f.findChild<QHBoxLayout*>("mechaduinoContainer");
//  mechaduinoContainer->addWidget(vfoPanel);

  QString configPath = "/home/user/projects/RemoteTubeAmplifaier/controller/config.conf";
  configPath = "config.conf";
  ApplicaionSettings& setting = ApplicaionSettings::getInstance();
  if(!setting.loadSettings(configPath))
    exit(0);

  StateModel::getInstance();
  Server server(setting.getServerPort());
  QThread serverThread;
  server.moveToThread(&serverThread);
  QObject::connect(&serverThread, &QThread::started, &server, &Server::doWork, Qt::QueuedConnection);
  serverThread.start();


//  IRadio* radio = RadioFactory::getRadio();

//  QObject::connect(radio, &IRadio::freqChanged, vfoPanel, &Vfo::vfoChangeFreq, Qt::QueuedConnection);

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
