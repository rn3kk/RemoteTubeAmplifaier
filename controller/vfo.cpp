#include "vfo.h"
#include "ui_vfo.h"

Vfo::Vfo(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::Vfo)
{
  ui->setupUi(this);
}

Vfo::~Vfo()
{
  delete ui;
}


void Vfo::vfoAChangeFreq(int newFreq)
{
  if(newFreq > 0)
    ui->VFO_A->setText(QString::number(newFreq));
  else
    disableAll();
}

void Vfo::vfoBChangeFreq(int newFreq)
{
  ui->VFO_B->setText(QString::number(newFreq));
}

void Vfo::vfoATX(quint8 tx)
{
  if(tx == 0)
  {
    ui->TX_A->setText("");
  }
  else
  {
    ui->TX_A->setText("TX");
  }
}

void Vfo::vfoBTX(quint8 tx)
{
  if(tx == 0)
  {
    ui->TX_B->setText("");
  }
  else
  {
    ui->TX_B->setText("TX");
  }
}

void Vfo::tuneMode(bool isTuneMode)
{
    ui->upFreqButton->setEnabled(isTuneMode);
    ui->downFreqButton->setEnabled(isTuneMode);
}

void Vfo::disableAll()
{
  ui->VFO_A->setText("-");
  ui->VFO_B->setText("-");
  ui->ActiveA->setText("");
  ui->ActiveB->setText("");
  ui->TX_A->setText("");
  ui->TX_B->setText("");
}

void Vfo::vfoAChangeActive(quint8 active)
{
  if(active ==0)
  {
    ui->ActiveA->setText("");
  }
  else
  {
    ui->ActiveA->setText("A");
  }
}

void Vfo::vfoBChangeActive(quint8 active)
{
  if(active ==0)
  {
    ui->ActiveB->setText("");
  }
  else
  {
    ui->ActiveB->setText("A");
  }
}

