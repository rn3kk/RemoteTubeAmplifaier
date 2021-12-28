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


void Vfo::vfoChangeFreq(int newFreq)
{
  if(newFreq > 0)
      ui->VFO_A->setText(QString::number(newFreq));
  else
    disableAll();
}

void Vfo::tuneMode(bool isTuneMode)
{
  m_tuneMode = isTuneMode;
  int freq = 0;
  if(!ui->VFO_A->text().isEmpty())
  {
    freq = ui->VFO_A->text().toInt();
    if(freq == -1)
      return;
  }  
  else
    return;

  ui->upFreqButton->setEnabled(isTuneMode);
  ui->downFreqButton->setEnabled(isTuneMode);

  int f(0);
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
    Q_EMIT tuneVFOTxToNewFreq(f);

}

void Vfo::disableAll()
{
  ui->VFO_A->setText("-");
  ui->ActiveA->setText("");
}

int Vfo::calculate25StepFreq()
{
  int newFreq;
  if(!ui->VFO_A->text().isEmpty())
  {
    newFreq = ui->VFO_A->text().toInt();
    if(newFreq == -1)
      return -1;
  } 
  else
    return 0;
  int freq = calculate25StepFreq(newFreq);
  return freq;
}

int Vfo::calculate25StepFreq(int newFreq)
{
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

void Vfo::on_upFreqButton_clicked()
{
  int f = calculate25StepFreq();
  if(f > 0)
  {
    f += 25;
    Q_EMIT savePosition();
    Q_EMIT tuneVFOTxToNewFreq(f);
  }
}

void Vfo::on_downFreqButton_clicked()
{
  int f = calculate25StepFreq();
  if(f > 25)
  {
    f -= 25;
    Q_EMIT savePosition();
    Q_EMIT tuneVFOTxToNewFreq(f);
  }
}
