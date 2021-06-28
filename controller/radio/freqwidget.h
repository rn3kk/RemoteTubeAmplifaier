#ifndef FREQWIDGET_H
#define FREQWIDGET_H

#include <QWidget>

namespace Ui {
class FreqWidget;
}

class FreqWidget : public QWidget
{
  Q_OBJECT

public:
  explicit FreqWidget(QWidget *parent = nullptr);
  ~FreqWidget();

private slots:
  void on_spinBox_valueChanged(int arg1);

signals:
  void freq(int);

private:
  Ui::FreqWidget *ui;
};

#endif // FREQWIDGET_H
