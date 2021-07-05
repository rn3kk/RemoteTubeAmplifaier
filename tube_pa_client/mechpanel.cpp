#include "mechpanel.h"
#include "ui_mechpanel.h"

MechPanel::MechPanel(const QString &panelName, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::MechPanel)
{
  ui->setupUi(this);
  ui->Panel->setTitle(panelName);
  ui->Panel->setStyleSheet("QGroupBox {"
                           "  border: 1px solid gray;"
                           "  border-radius: 9px;"
                           "  margin-top: 0.5em;"
                           "}");
  ui->dial->setEnabled(false);
}

MechPanel::~MechPanel()
{
  delete ui;
}

QString MechPanel::getName()
{
  return ui->Panel->title();
}

void MechPanel::position(int position)
{
  ui->dial->blockSignals(true);
  ui->lcdNumber->display(position);
  ui->dial->setValue(position);
  ui->dial->blockSignals(false);
}

void MechPanel::tuneMode(bool tuneMode)
{
  ui->dial->setEnabled(tuneMode);
  ui->manualSettingsCheckBox->setEnabled(tuneMode);
}

void MechPanel::setManualMode(bool mode)
{
  ui->manualSettingsCheckBox->setCheckState(mode?Qt::CheckState::Checked : Qt::CheckState::Unchecked);
  ui->dial->setEnabled(!mode);
}

void MechPanel::on_Panel_objectNameChanged(const QString &objectName)
{

}

int lastValue;
void MechPanel::on_dial_valueChanged(int value)
{
  if (lastValue == value)
      return;

  lastValue = value;
  Mechaduino m;
  m.name = getName();
  m.position = ui->dial->value();
  m.manualMode = ui->manualSettingsCheckBox->isChecked();
   Q_EMIT changed(m);
}

void MechPanel::on_manualSettingsCheckBox_clicked(bool checked)
{
  ui->dial->setEnabled(!checked);
  Mechaduino m;
  m.name = getName();
  m.position = ui->dial->value();
  m.manualMode = ui->manualSettingsCheckBox->isChecked();
  Q_EMIT changed(m);
}
