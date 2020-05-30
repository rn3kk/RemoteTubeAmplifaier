#ifndef VFO_H
#define VFO_H

#include <QWidget>

namespace Ui {
class Vfo;
}

class Vfo : public QWidget
{
  Q_OBJECT

public:
  explicit Vfo(QWidget *parent = nullptr);
  ~Vfo();

public slots:
  void vfoAChangeFreq(const QString& newFreq);
  void vfoBChangeFreq(const QString& newFreq);
  void vfoAChangeActive(quint8 active); // 0 or 1
  void vfoBChangeActive(quint8 active); // 0 or 1
  void vfoATX(quint8 tx);
  void vfoBTX(quint8 tx);

private:
  Ui::Vfo *ui;
};

#endif // VFO_H
