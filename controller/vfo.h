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
  void vfoAChangeFreq(int newFreq);
  void vfoBChangeFreq(int newFreq);
  void vfoAChangeActive(quint8 active); // 0 or 1
  void vfoBChangeActive(quint8 active); // 0 or 1
  void vfoATX(quint8 tx);
  void vfoBTX(quint8 tx);
  void tuneMode(bool isTuneMode);

signals:
  void tuneTxSliceToNewFreq(int frequency);

private slots:
  void on_upFreqButton_clicked();
  void on_downFreqButton_clicked();

private:
  void disableAll();
  int calculate25StepFreq();
private:
  Ui::Vfo *ui;
};

#endif // VFO_H
