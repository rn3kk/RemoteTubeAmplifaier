#ifndef PROTOKOL_H
#define PROTOKOL_H

#include <QByteArray>

class JsonProtokol
{
public:
  static bool isRequest(const QByteArray& data);
  static bool checkToken(const QByteArray& data);

};

#endif // PROTOKOL_H
