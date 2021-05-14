#ifndef FORM_H
#define FORM_H

#include <QWidget>

namespace Ui {
class Form;
class MechPanel;
}

class Form : public QWidget
{
  Q_OBJECT

public:
  explicit Form(QWidget *parent = nullptr);
  ~Form();

  void addMechpanel(Ui::MechPanel* panel);

signals:
  void setFreq(float freq);
  void tuneMode(bool isTuneMode); //true enable tune; false disable tune mode

private slots:
  void on_tuneButton_clicked();

  void on_pushButton_clicked();

private:
  Ui::Form *ui;
  bool m_tuneMode = false;
};

#endif // FORM_H
