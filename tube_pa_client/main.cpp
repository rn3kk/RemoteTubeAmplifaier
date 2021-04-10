#include <QApplication>
#include <QThread>
#include "mainwindow.h"
#include "ampitemwidget.h"
#include "broadcastreceiver.h"

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

  return a.exec();
}
