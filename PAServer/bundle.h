#ifndef BUNDLE_H
#define BUNDLE_H

#include <QObject>
#include <QString>
#include <QList>
#include "sockethandler.h"

class Bundle final : public QObject
{
  Q_OBJECT
public:
  Bundle(SocketHandler *handler);
  ~Bundle();

  void addSocketHandler(SocketHandler* handler);
  QString token() const;

private:
  bool handlersIsEmpty();

private slots:
  void socketHandlerDestroyed();

signals:
  void bundleIsEmpty();

private:
  QString m_token;
  SocketHandler * m_paHandler;
  QList<SocketHandler*> m_clients;
};

#endif // BUNDLE_H
