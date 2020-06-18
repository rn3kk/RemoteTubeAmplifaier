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
}

MechPanel::~MechPanel()
{
  delete ui;
}

void MechPanel::newPosition(int position)
{
  ui->lcdNumber->display(position);
  ui->dial->setValue(position);
}

void MechPanel::on_Panel_objectNameChanged(const QString &objectName)
{

}

void MechPanel::on_dial_sliderMoved(int position)
{
  ui->lcdNumber->display(position);
  Q_EMIT changePosition(position);
}

void MechPanel::on_dial_valueChanged(int value)
{
  // ui->lcdNumber->display(value);
   //Q_EMIT changePosition(value);
}
