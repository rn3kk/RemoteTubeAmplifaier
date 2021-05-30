#ifndef PRESENTOR_H
#define PRESENTOR_H

#include <QObject>

class Presentor : public QObject
{
  Q_OBJECT
public:
  explicit Presentor(QObject *parent = nullptr);

signals:

private:


};

#endif // PRESENTOR_H
