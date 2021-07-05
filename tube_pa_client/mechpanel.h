#ifndef MECHPANEL_H
#define MECHPANEL_H

#include <QTimerEvent>
#include <QWidget>
#include "../common/model.h"

namespace Ui {
class MechPanel;
}

class MechPanel : public QWidget
{
  Q_OBJECT

public:
  explicit MechPanel(const QString& panelName, QWidget *parent = nullptr);
  ~MechPanel();

  QString getName();
  void tuneMode(bool tuneMode);
  void setManualMode(bool mode);

signals:
  void changed(const Mechaduino& m);

public slots:
  void position(int position);

private slots:
  void on_Panel_objectNameChanged(const QString &objectName);
  void on_dial_valueChanged(int value);
  void on_manualSettingsCheckBox_clicked(bool checked);

private:
  Ui::MechPanel *ui;
};

#endif // MECHPANEL_H
