#include <QDateTime>
#include <QDebug>
#include <QShowEvent>
#include <QCloseEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include "../common/socketwrapper.h"
#include "../common/common.h"
#include "form.h"
#include "ui_form.h"
#include "mechpanel.h"

Form::Form(QString name, QString ip, quint16 port, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::Form),
  m_name(name),
  m_ip(ip),
  m_port(port)
{
  ui->setupUi(this);
  m_lineEdit = ui->lineEdit;

}

Form::~Form()
{
  if(m_client)
    delete m_client;
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

void Form::on_pushButton_clicked()
{
  QString freq = ui->lineEdit->text();
  Q_EMIT setFreq(freq.toFloat());
}

void Form::remoteModelIsChanged(const QByteArray &data)
{
  QJsonParseError jsonError;
  QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
  if (jsonError.error != QJsonParseError::NoError)
  {
    qDebug() << jsonError.errorString();
    return;
  }
  QJsonObject obj = doc.object();

  QJsonValue val = obj.value(FREQ);
  QString str = QString::number(val.toDouble());
  m_lineEdit->setText(str);

}

void Form::showEvent(QShowEvent *event)
{
  switch (event->type()) {
  case QShowEvent::Show:
    m_client = new SocketWrapper(m_ip, m_port);
    connect(m_client, &SocketWrapper::avaliableData, this, &Form::remoteModelIsChanged);
    break;
  }
}

void Form::closeEvent(QCloseEvent* event)
{
  if(m_client)
  {
    delete m_client;
    m_client = nullptr;
  }
}
