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

void Form::on_startTuneButton_clicked()
{
    QDateTime d = QDateTime::currentDateTime();
    qDebug() << "on_startTuneButton_clicked " << d;
    int p = ui->lineEdit->text().toInt();
    Q_EMIT setPosition(p);
}
