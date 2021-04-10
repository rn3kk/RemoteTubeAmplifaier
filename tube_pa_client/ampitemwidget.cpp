#include "ampitemwidget.h"
#include "ui_ampitemwidget.h"

AmpItemWidget::AmpItemWidget(QString ip, quint16 port, QString paName, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::AmpItemWidget),
  m_ip(ip),
  m_port(port),
  m_name(paName)
{
  ui->setupUi(this);
  ui->labelIPAddres->setText(ip);
  ui->paNameLabel->setText(paName);
  setNewTime();
}

AmpItemWidget::~AmpItemWidget()
{
  delete ui;
}

void AmpItemWidget::on_connectButton_clicked()
{

}

QDateTime AmpItemWidget::updateTime() const
{
  return m_updateTime;
}

void AmpItemWidget::setNewTime()
{
  m_updateTime = QDateTime::currentDateTime();
  ui->timeLabel->setText(m_updateTime.toString("hh:mm:ss"));
}

QString AmpItemWidget::name() const
{
  return m_name;
}

bool AmpItemWidget::operator==(AmpItemWidget* w)
{
  if(w->ip() == m_ip &&
     w->name() == m_name &&
     w->port() == m_port)
    return true;
  return false;
}

quint16 AmpItemWidget::port() const
{
  return m_port;
}

QString AmpItemWidget::ip() const
{
  return m_ip;
}
