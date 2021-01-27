#include <QThread>
#include <QLoggingCategory>
#include "typetokenparser.h"
#include "bundle.h"
#include "sockethandlermanager.h"

QLoggingCategory shmCat("SocketHandlerManager");

SocketHandlerManager &SocketHandlerManager::getInstance()
{
  static SocketHandlerManager instance;
  return instance;
}

SocketHandlerManager::SocketHandlerManager()
{
  qCDebug(shmCat) << "SocketHandlerManager()";
//  QThread* th = new QThread();
//  this->moveToThread(th);
//  th->start();
}

SocketHandlerManager::~SocketHandlerManager()
{
qCDebug(shmCat) << "~SocketHandlerManager()";
}

void SocketHandlerManager::addSocketHandler(SocketHandler *handler)
{
  qCDebug(shmCat) << "addSocketHandker";
  if(handler->type() == TypeToken::UNKN)
  {
    handler->destroy();
    return;
  }

  for(Bundle* b: m_bundles)
  {
    if(b->token().compare(handler->token()) == 0)
    {
      b->addSocketHandler(handler);
      return;
    }
  }
  Bundle* b = new Bundle(handler);
  b->addSocketHandler(handler);
  connect(b, &Bundle::bundleIsEmpty, this, &SocketHandlerManager::bundleIsEmpty, Qt::QueuedConnection);
  m_bundles.append(b);
}

void SocketHandlerManager::bundleIsEmpty()
{
  qCDebug(shmCat) << "bundleIsEmpty";
  Bundle * b = qobject_cast<Bundle*>(sender());
  m_bundles.removeAll(b);
  delete b;
}
