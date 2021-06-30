#ifndef PINSWORKER_H
#define PINSWORKER_H

#include <QObject>

class PinsWorker : public QObject
{
  Q_OBJECT
public:
  static PinsWorker& getInstance();
signals:

public slots:

private:
  explicit PinsWorker(QObject *parent = nullptr);

};

#endif // PINSWORKER_H
