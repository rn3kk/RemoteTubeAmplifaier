#include "freqwidget.h"
#include "ui_freqwidget.h"

FreqWidget::FreqWidget(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FreqWidget)
{
  ui->setupUi(this);
}

FreqWidget::~FreqWidget()
{
  delete ui;
}

void FreqWidget::on_spinBox_valueChanged(int arg1)
{
    Q_EMIT freq(arg1);
}
