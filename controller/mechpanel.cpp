#include "mechpanel.h"
#include "ui_mechpanel.h"

MechPanel::MechPanel(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::MechPanel)
{
  ui->setupUi(this);
}

MechPanel::~MechPanel()
{
  delete ui;
}

void MechPanel::on_Panel_objectNameChanged(const QString &objectName)
{

}

void MechPanel::on_dial_sliderMoved(int position)
{

}
