#include <QApplication>
#include <QThread>
#include "mainwindow.h"
#include "ampitemwidget.h"
#include "broadcastreceiver.h"
#include "../common/socketwrapper.h"

int main(int argc, char *argv[])
{
  qRegisterMetaType<AmpInfo>("AmpInfo");

  QApplication a(argc, argv);
  MainWindow m;
  m.show();

  BroadcastReceiver bc;
  QThread th;
  bc.moveToThread(&th);
  QObject::connect(&th, &QThread::started, &bc, &BroadcastReceiver::start, Qt::QueuedConnection);
  th.start();

  QObject::connect(&bc, &BroadcastReceiver::findAmplifaier, &m, &MainWindow::updateLockalAmp);

  SocketWrapper sw("127.0.0.1", 5993);

  return a.exec();
}
