#ifndef PROTOKOL_H
#define PROTOKOL_H

#include <QString>
#include <QByteArray>
#include <QPair>

class JsonProtokol
{
public:
  static bool isRequest(const QByteArray& data);
  static bool checkToken(const QByteArray& data);
  static QByteArray createChangeRequest(QString key, QString newValue);
  static QPair<QString, QString> parceChangeRequest(QByteArray data);

};

#endif // PROTOKOL_H
