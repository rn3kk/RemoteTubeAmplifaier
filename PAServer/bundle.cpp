#include <QLoggingCategory>
#include "bundle.h"

QLoggingCategory bundlecat("Bundle");

Bundle::Bundle(SocketHandler *handler):
  m_paHandler(nullptr)
{
  m_token = handler->token();
  addSocketHandler(handler);
  qCDebug(bundlecat) << "Bundle() for token" << m_token;
}

Bundle::~Bundle()
{
  qCDebug(bundlecat) << "~Bundle() for token" << m_token;
}

void Bundle::addSocketHandler(SocketHandler *handler)
{
  if(handler->type() == TypeToken::PA_CLIENT)
  {
    if(m_paHandler != nullptr)
    {
      qCDebug(bundlecat) << "PA hanlder also exist. Destroy here.";
      m_paHandler->destroy();
    }
    m_paHandler = handler;
  }
  else if(handler->type() == TypeToken::SOFT_CLIENT)
  {
    if(m_clients.count() > 3)
    {
      for(SocketHandler* h:m_clients)
      {
        h->destroy();
      }
      m_clients.clear();
    }
    m_clients.append(handler);
  }
  connect(handler, &SocketHandler::finished, this, &Bundle::socketHandlerDestroyed, Qt::QueuedConnection);
}

bool Bundle::handlersIsEmpty()
{
  if(m_paHandler == nullptr && m_clients.isEmpty())
    return true;
  return false;
}

QString Bundle::token() const
{
  return m_token;
}

void Bundle::socketHandlerDestroyed()
{
  SocketHandler* handler =  qobject_cast<SocketHandler*>(sender());
  if(m_paHandler == handler)
    m_paHandler = nullptr;
  else
    m_clients.removeAll(handler);

  if(handlersIsEmpty())
    emit bundleIsEmpty();
}

