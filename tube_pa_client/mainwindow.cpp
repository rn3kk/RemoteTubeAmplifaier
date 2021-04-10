#include "mainwindow.h"
#include "ampitemwidget.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  m_list = ui->ampsListItems;
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::updateLockalAmp(const AmpInfo &ai)
{
  AmpItemWidget* ampWidget = new AmpItemWidget(ai.paIp(), ai.paPort(), ai.paName());
  if(m_list->count() == 0)
  {
    QListWidgetItem *item = new QListWidgetItem();
    item->setSizeHint(QSize(ampWidget->width(), ampWidget->height()));
    m_list->addItem(item);
    m_list->setItemWidget(item, ampWidget);
    return;
  }

  for(int i = 0; i < m_list->count(); ++i)
  {
    QListWidgetItem* item = m_list->item(i);
    if(item)
    {
      AmpItemWidget *w = dynamic_cast<AmpItemWidget*>(m_list->itemWidget(item));
      if(w->operator==(ampWidget))
      {
        w->setNewTime();
        continue;
      }
      else
      {
        QListWidgetItem *item = new QListWidgetItem();
        item->setSizeHint(QSize(ampWidget->width(), ampWidget->height()));
        m_list->addItem(item);
        m_list->setItemWidget(item, ampWidget);
        return;
      }
    }

    if( (i+1) ==  m_list->count()) //exit if not find
    {
      if(ampWidget)
        delete ampWidget;
      return;
    }
  }

}
