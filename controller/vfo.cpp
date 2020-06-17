#include <cmath>
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
  int freq = 0;
  if(!ui->TX_A->text().isEmpty())
  {
    freq = ui->VFO_A->text().toInt();
  }
  else if(!ui->TX_B->text().isEmpty())
  {
    freq = ui->VFO_B->text().toInt();
  }
  else
    return;

  ui->upFreqButton->setEnabled(isTuneMode);
  ui->downFreqButton->setEnabled(isTuneMode);

  int pos, f(0);
  int begin = freq/100; // отбрасываем последние два числа
  begin *= 100;
  int end = freq % 100; //остаток от деления есть два последних числа частоты
  if(end >= 0 && end < 25)
    f = begin;
  else if(end >= 25 && end < 50)
    f = (begin + 25);
  else if(end >= 50 && end < 75)
    f = (begin + 50);
  else if(end >= 75 && end <100)
    f = (begin + 75);

  if(f > 0)
    Q_EMIT tuneTxSliceToNewFreq(f);

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

int Vfo::calculate25StepFreq()
{
  int newFreq;
  if(!ui->TX_A->text().isEmpty())
  {
    newFreq = ui->VFO_A->text().toInt();
  }
  else if(!ui->TX_B->text().isEmpty())
  {
    newFreq = ui->VFO_B->text().toInt();
  }
  else
    return 0;

  int f(0);
  int begin = newFreq/100; // отбрасываем последние два числа
  begin *= 100;
  int end = newFreq % 100; //остаток от деления есть два последних числа частоты
  if(end >= 0 && end < 25)
    f = begin;
  else if(end >= 25 && end < 50)
    f = (begin + 25);
  else if(end >= 50 && end < 75)
    f = (begin + 50);
  else if(end >= 75 && end <100)
    f = (begin + 75);

  return f;
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

void Vfo::on_upFreqButton_clicked()
{
  int f = calculate25StepFreq();
  if(f > 0)
  {
    f += 25;
    Q_EMIT tuneTxSliceToNewFreq(f);
  }

}

void Vfo::on_downFreqButton_clicked()
{
  int f = calculate25StepFreq();
  if(f > 25)
  {
    f -= 25;
    Q_EMIT tuneTxSliceToNewFreq(f);
  }
}
