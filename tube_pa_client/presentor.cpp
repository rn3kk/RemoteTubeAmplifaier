#include "form.h"
#include "../common/socketwrapper.h"
#include "../common/commands.h"
#include "presentor.h"

Presentor::Presentor(QString name, QString ip, QString port, QObject *parent) : QObject(parent), m_name(name)
{
  m_form = new Form(name, ip, port);
  m_client = new SocketWrapper(ip, port.toInt() );
  connect(m_client, &SocketWrapper::avaliableData, &m_model, &FrontModel::fromJson);
  connect(m_client, &SocketWrapper::socketDiskonnected, m_form, &Form::close);
  connect(&m_model, &FrontModel::modelChanged, this, &Presentor::modelChanged);
  connect(m_form, &Form::destroyed,this, &Presentor::deleteLater);
  connect(m_form, &Form::pwr,this, &Presentor::needChangePWR);
  connect(m_form, &Form::tuneMode,this, &Presentor::needChangeTuneMode);
  connect(m_form, &Form::relay,this, &Presentor::needChangeRelay);
  connect(m_form, &Form::mechPos,this, &Presentor::needChangeMechaduino);
  m_form->show();

}

Presentor::~Presentor()
{
  m_form = nullptr;
  qDebug() << "~Presentor()";
  if(m_client)
    delete m_client;
}

void Presentor::modelChanged()
{
  if(m_form)
  {
    m_form->setPwrState(m_model.power());
    m_form->setTuneMode(m_model.tuneMode());
    m_form->setFreq(m_model.radioFreq());
    QMap<QString, int> ms = m_model.mechaduinoStates();
    for(QMap<QString, int>::const_iterator it = ms.constBegin(); it != ms.constEnd(); it++)
    {
      m_form->changeMechPanel(it.key(), it.value());
    }
  }
}

void Presentor::needChangePWR()
{
  qDebug() << "needChangePWR";
  QByteArray cmd = Commands::changePwr(!m_model.power());
  m_client->writeToSocket(cmd);
}

void Presentor::needChangeTuneMode()
{
  qDebug() << "needChangeTuneMode to " << !m_model.tuneMode();
  QByteArray cmd = Commands::changeTune(!m_model.tuneMode());
  m_client->writeToSocket(cmd);
}

void Presentor::needChangeRelay(int relay)
{
  qDebug() << "needChangeRelay";
  QByteArray cmd = Commands::changeRelay(relay);
  m_client->writeToSocket(cmd);
}

void Presentor::needChangeMechaduino(QPair<QString, int> mech)
{
  qDebug() << "needChangeMechaduino";
  QByteArray cmd = Commands::changeMech(mech.first, QString::number(mech.second));
  m_client->writeToSocket(cmd);
}
