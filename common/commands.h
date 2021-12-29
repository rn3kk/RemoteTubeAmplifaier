#ifndef PROTOKOL_H
#define PROTOKOL_H

#include <QString>
#include <QByteArray>
#include <QPair>
#include <QVariant>
#include "../common/model.h"

class Commands
{
public:

  static QByteArray createMessage(int cmd, int value);
  static bool parseMessage(const QByteArray& data, int& cmd, int& value);  

};

#endif // PROTOKOL_H
