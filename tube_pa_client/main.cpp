#include <QApplication>
#include <QThread>
#include "mainwindow.h"
#include "broadcastreceiver.h"

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
//  MainWindow m;
//  m.show();

  BroadcastReceiver bc;
  QThread th;
  bc.moveToThread(&th);
  QObject::connect(&th, &QThread::started, &bc, &BroadcastReceiver::start, Qt::QueuedConnection);
  th.start();

  return a.exec();
}
