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
  m_mechPanels.clear();
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

void Form::on_tuneButton_clicked()
{  
  Q_EMIT tuneMode();
}

void Form::on_pwrButton_clicked()
{
  Q_EMIT pwr();
}

void Form::closeEvent(QCloseEvent* event)
{
  this->deleteLater();
}

void Form::setPwrState(bool state)
{
  QPalette pal = m_pwrButton->palette();
  if(state)
  {
    pal.setColor(QPalette::Button, QColor(Qt::blue));
    m_pwrButton->setText("ВКЛ");
  }
  else
  {
    pal.setColor(QPalette::Button, QColor(Qt::green));
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
  for(MechPanel* p: m_mechPanels)
  {
    p->tuneMode(state);
  }
}

void Form::setMechaduinoParams(const Mechaduino& m)
{
  bool found=false;
  for(MechPanel* p: m_mechPanels)
  {
    if(p->getName().compare(m.name) == 0)
    {
      found=true;
      p->position(m.position);
      p->setManualMode(m.manualMode);
    }
  }
  if(found)
    return;
  else
  {
    MechPanel* panel = new MechPanel(m.name);
    panel->position(m.position);
    panel->setManualMode(m.manualMode);
    panel->tuneMode(m_tuneMode);
    ui->mechaduinoContainer->addWidget(panel);
    connect(panel, &MechPanel::changed, this, &Form::mechChanged);
    m_mechPanels.append(panel);
  }
}
