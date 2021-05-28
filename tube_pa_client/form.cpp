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
#include "../common/socketwrapper.h"
#include "../common/common.h"
#include "../common/jsonprotokol.h"
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
  m_pwrButton = ui->pwrButton;
  m_pwrButton->setAutoFillBackground(true);
  m_mechPanels.clear();
}

Form::~Form()
{
  if(m_client)
    delete m_client;
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

void Form::on_tuneButton_clicked()
{  
  QByteArray changeReq = JsonProtokol::createChangeRequest(TUNE_MODE, QString::number(!m_tuneMode));
  Q_EMIT sendRequest(changeReq);
}

void Form::on_pwrButton_clicked()
{
  QByteArray changeReq = JsonProtokol::createChangeRequest(POWER, QString::number(!m_pwr));
  Q_EMIT sendRequest(changeReq);
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
  if(!str.isEmpty())
    m_lineEdit->setText(str);

  val = obj.value(POWER);
  if(!val.isNull())
    setPwrState(QVariant(val.toString()).toBool());

  val = obj.value(TUNE_MODE);
  if(!val.isNull())
    setTuneMode(QVariant(val.toString()).toBool());

  QJsonArray mechArr = obj[MECH].toArray();
  for(const QJsonValue value: mechArr)
  {
    QJsonObject mechObj = value.toObject();
    QString name = mechObj.keys().at(0);
    int position = mechObj.value(name).toInt(-1);
    changeMechPanel(name, position);
  }

}

void Form::needChangeMechPos(int pos)
{
  MechPanel* panel = qobject_cast<MechPanel*>(sender());
  QByteArray changeReq = JsonProtokol::createChangeRequest(MECH, panel->getName() + SEPARATOR+QString::number(pos));
  Q_EMIT sendRequest(changeReq);
}

void Form::showEvent(QShowEvent *event)
{
  switch (event->type()) {
  case QShowEvent::Show:
    m_client = new SocketWrapper(m_ip, m_port);
    connect(m_client, &SocketWrapper::avaliableData, this, &Form::remoteModelIsChanged);
    connect(this, &Form::sendRequest, m_client, &SocketWrapper::writeToSocket);
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

void Form::setPwrState(bool state)
{
  if(m_pwr == state)
    return;

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
  if(m_tuneMode== state)
    return;
  else if(state)
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
