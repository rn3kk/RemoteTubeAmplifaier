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

void Form::changeMechPanel(QString name, int pos)
{
  MechPanel* panel;
  if(m_mechPanels.contains(name))
    m_mechPanels.value(name)->newPosition(pos);
  else
  {
    panel = new MechPanel(name);
    panel->newPosition(pos);
    connect(this, &Form::tuneMode, panel, &MechPanel::tuneMode);
    connect(panel, &MechPanel::changePosition, this, &Form::needChangeMechPos);
    m_mechPanels[name] = panel;
    ui->mechaduinoContainer->addWidget(panel);
  }
}

void Form::setFreq(float freq)
{
  m_freqEdit->setText(QString::number(freq));
}

void Form::on_tuneButton_clicked()
{  
  Q_EMIT tuneMode(!m_tuneMode);
}

void Form::on_pwrButton_clicked()
{
  Q_EMIT pwr(!m_pwr);
}

void Form::needChangeMechPos(int pos)
{
  MechPanel* panel = qobject_cast<MechPanel*>(sender());
  QPair<QString, int> p;
  p.first = panel->getName();
  p.second = pos;
  Q_EMIT mechPos(p);
}


void Form::closeEvent(QCloseEvent* event)
{
  this->deleteLater();
}

void Form::setPwrState(bool state)
{
  m_pwr = state;
  QPalette pal = m_pwrButton->palette();
  if(m_pwr)
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
  if(!state)
  {
    ui->tuneButton->setText("Tune");
    m_tuneMode = true;
    Q_EMIT tuneMode(m_tuneMode);
  }
  else
  {
    ui->tuneButton->setText("End tune");
    m_tuneMode = false;
    Q_EMIT tuneMode(m_tuneMode);
  }
}
