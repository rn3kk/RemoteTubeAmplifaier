#include <QCoreApplication>
#include <QTextStream>
#include <QFile>
#include <QDateTime>
#include <QThread>
#include <iostream>
#include "sockethandlermanager.h"
#include "server.h"

static QFile logFile("paserver.log");

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QTextStream out(&logFile);
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ");
    switch (type)
    {
    case QtInfoMsg:     out << "INF "; break;
    case QtDebugMsg:    out << "DBG "; break;
    case QtWarningMsg:  out << "WRN "; break;
    case QtCriticalMsg: out << "CRT "; break;
    case QtFatalMsg:    out << "FTL "; break;
    }
    out << context.category << ": "
        << msg << endl;
    out.flush();
}

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);

  if(!logFile.open(QFile::Append | QFile::Text))
    std::cout << "can't open log file";
  qInstallMessageHandler(messageHandler);

  Server server;
  SocketHandlerManager::getInstance();

  return a.exec();
}


