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
  void vfoChangeFreq(int newFreq);
  void tuneMode(bool isTuneMode);

signals:
  void tuneVFOTxToNewFreq(int frequency);
  void savePosition();

private slots:
  void on_upFreqButton_clicked();
  void on_downFreqButton_clicked();

private:
  void disableAll();
  int calculate25StepFreq();
  int calculate25StepFreq(int freq);

private:
  Ui::Vfo *ui;
  bool m_tuneMode = false;
};

#endif // VFO_H
