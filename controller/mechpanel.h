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
  void manualMode(bool manualMode);

public slots:
  void newPosition(int position);
  void tuneMode(bool tuneMode);  

private slots:
  void on_Panel_objectNameChanged(const QString &objectName);
  void on_dial_sliderMoved(int position);
  void on_dial_valueChanged(int value);

  void on_manualSettingsCheckBox_stateChanged(int arg1);

  void on_manualSettingsCheckBox_clicked(bool checked);

private:
  Ui::MechPanel *ui;
  bool m_tuneMode = false;
};

#endif // MECHPANEL_H
