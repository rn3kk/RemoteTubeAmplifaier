#ifndef PROTOKOL_H
#define PROTOKOL_H

#include <QString>
#include <QByteArray>
#include <QPair>
#include <QVariant>

class Commands
{
public:
  enum Type
  {
    CHANGE_POWER = 0,
    CHANGE_TUNE,
    CHANGE_RELAY,
    CHANGE_MECH,
    UNKW
  };

  static bool isRequest(const QByteArray& data);
  //static bool checkToken(const QByteArray& data);
  static QByteArray createChangeRequest(QString key, QString newValue);
  static QPair<QString, QString> parceChangeRequest(QByteArray data);

  static Type getType(const QByteArray& data);
  static QString getValue(const QByteArray& data);
  static QPair<QString, int> getMechaduinoPos(const QByteArray& data);

  static QByteArray changePwr(bool pwr);
  static QByteArray changeTune(bool tune);
  static QByteArray changeRelay(int relay);
  static QByteArray changeMech(QString mech, QString pos);

private:
  static QByteArray createReq(QString req, QString value);

};

#endif // PROTOKOL_H
