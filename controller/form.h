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

private:
  Ui::Form *ui;
};

#endif // FORM_H
