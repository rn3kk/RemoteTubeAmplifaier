#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include "../common/ampinfo.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void timerEvent(QTimerEvent* event) override;


public slots:
  void updateLockalAmp(const AmpInfo& ai);




private:
  Ui::MainWindow *ui;
  QListWidget *m_list;
};

#endif // MAINWINDOW_H
