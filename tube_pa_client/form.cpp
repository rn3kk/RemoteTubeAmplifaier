#include <QDateTime>
#include <QDebug>
#include <QShowEvent>
#include <QCloseEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QPushButton>
#include <QJsonArray>
#include <QHBoxLayout>
#include "../common/common.h"
#include "../common/commands.h"
#include "form.h"
#include "ui_form.h"
#include "mechpanel.h"

Form::Form(QString name, QString ip, QString port, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::Form)
{
  ui->setupUi(this);
  m_freqEdit = ui->freqEdit;
  m_pwrButton = ui->pwrButton;
  m_pwrButton->setAutoFillBackground(true);
  m_mech_panel1 = new MechPanel("Tune");
  m_mech_panel2 = new MechPanel("Load");
  connect(m_mech_panel1, &MechPanel::changed, this, &Form::mechChanged);
  connect(m_mech_panel2, &MechPanel::changed, this, &Form::mechChanged);
  ui->mechaduinoContainer->addWidget(m_mech_panel1);
  ui->mechaduinoContainer->addWidget(m_mech_panel2);
  setWindowTitle(name + " " + ip + ":" + port);
}

Form::~Form()
{
  qDebug() << "~Form()";
  delete ui;
}

void Form::setFreq(float freq)
{
  m_freqEdit->setText(QString::number(freq));
}

void Form::setProtectionStatus(int status)
{

}

void Form::on_tuneButton_clicked()
{  
  Q_EMIT tuneMode();
}

void Form::on_pwrButton_clicked()
{
    Q_EMIT pwr();
}

void Form::closeEvent(QCloseEvent *event)
{
  this->deleteLater();
}

void Form::setPwrState(bool state)
{
  QPalette pal = m_pwrButton->palette();
  if(state)
  {
    pal.setColor(QPalette::Button, QColor(Qt::green));
    m_pwrButton->setText("ВКЛ");
  }
  else
  {
    pal.setColor(QPalette::Button, QColor(Qt::gray));
    m_pwrButton->setText("ВЫКЛ");
  }

  m_pwrButton->setPalette(pal);
  m_pwrButton->update();
}

void Form::setTuneMode(bool state)
{
  m_tuneMode = state;
  if(!state)
  {
    ui->tuneButton->setText("Tune");
  }
  else
  {
    ui->tuneButton->setText("End tune");
  }
//  for(MechPanel* p: m_mechPanels)
//  {
//    p->tuneMode(state);
//  }
}

void Form::setMechaduinoParams(int mech1_pos, int mech2_pos)
{
  m_mech_panel1->position(mech1_pos);
  m_mech_panel2->position(mech2_pos);
}
