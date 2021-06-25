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
  }
}

void Presentor::needChangePWR(bool pwr)
{
  qDebug() << "needChangePWR";
  QByteArray cmd = Commands::changePwr(pwr);
  m_client->writeToSocket(cmd);
}
