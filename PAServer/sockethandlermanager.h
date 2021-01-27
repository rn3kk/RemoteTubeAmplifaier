#ifndef SOCKETHANDLERMANAGER_H
#define SOCKETHANDLERMANAGER_H

#include <QObject>
#include <QList>
#include <QVector>
#include "sockethandler.h"

class Bundle;

class SocketHandlerManager final : public QObject
{
  Q_OBJECT
public:
  static SocketHandlerManager& getInstance();

public slots:
  void addSocketHandler(SocketHandler* handler);


private slots:
  void bundleIsEmpty();

private:
  SocketHandlerManager();
  ~SocketHandlerManager();

private:
  QVector<Bundle*> m_bundles;

};

#endif // SOCKETHANDLERMANAGER_H
