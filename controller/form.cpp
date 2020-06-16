#include <QDateTime>
#include <QDebug>
#include "form.h"
#include "ui_form.h"
#include "mechpanel.h"

Form::Form(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::Form)
{
  ui->setupUi(this);

}

Form::~Form()
{
  delete ui;
}

void Form::addMechpanel(Ui::MechPanel *panel)
{

}

void Form::on_tuneButton_clicked()
{
  if(m_tuneMode)
  {
    ui->tuneButton->setText("Tune");
    m_tuneMode = false;
    Q_EMIT tuneMode(m_tuneMode);
  }
  else
  {
    ui->tuneButton->setText("End tune");
    m_tuneMode = true;
    Q_EMIT tuneMode(m_tuneMode);
  }
}
