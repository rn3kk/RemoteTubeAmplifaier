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
  explicit MechPanel(const QString& panelName, QWidget *parent = nullptr);
  ~MechPanel();

signals:
  void changePosition(qint64 position);

public slots:
  void newPosition(int position);

private slots:
  void on_Panel_objectNameChanged(const QString &objectName);
  void on_dial_sliderMoved(int position);
  void on_dial_valueChanged(int value);

private:
  Ui::MechPanel *ui;
};

#endif // MECHPANEL_H
