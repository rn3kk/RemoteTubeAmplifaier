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
  void addSocketHandler(SocketHandler* handler);

private:
  SocketHandlerManager();
  ~SocketHandlerManager();

private slots:
  void bundleIsEmpty();

private:
  QVector<Bundle*> m_bundles;

};

#endif // SOCKETHANDLERMANAGER_H
