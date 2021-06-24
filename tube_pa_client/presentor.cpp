#include "form.h"
#include "../common/socketwrapper.h"
#include "presentor.h"

Presentor::Presentor(QString name, QString ip, QString port, QObject *parent) : QObject(parent), m_name(name)
{

  m_form = new Form(name, ip, port.toInt());
  m_client = new SocketWrapper(ip, port.toInt() );
  //connect(m_client, &SocketWrapper::avaliableData, this, &Form::remoteModelIsChanged);

}

Presentor::~Presentor()
{
  if(m_client)
    delete m_client;
}
