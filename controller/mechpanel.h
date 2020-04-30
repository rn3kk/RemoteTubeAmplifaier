#ifndef MECHPANEL_H
#define MECHPANEL_H

#include <QWidget>

namespace Ui {
class MechPanel;
}

class MechPanel : public QWidget
{
  Q_OBJECT

public:
  explicit MechPanel(QWidget *parent = nullptr);
  ~MechPanel();

private slots:
  void on_Panel_objectNameChanged(const QString &objectName);

  void on_dial_sliderMoved(int position);

private:
  Ui::MechPanel *ui;
};

#endif // MECHPANEL_H
