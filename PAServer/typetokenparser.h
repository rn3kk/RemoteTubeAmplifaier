#ifndef TOKEN_H
#define TOKEN_H

#include <QByteArray>


class TypeToken
{
public:

  enum Type
  {
    PA_CLIENT,
    SOFT_CLIENT,
    UNKN
  };

  TypeToken();
  TypeToken(TypeToken& token);
  virtual ~TypeToken();
  bool parseTokenAndClientType(QByteArray data);
  QByteArray token() const;
  Type type() const;
  QString getTypeString() const;

private:
  QByteArray m_token;
  Type m_type = UNKN;

};

#endif // TOKEN_H
