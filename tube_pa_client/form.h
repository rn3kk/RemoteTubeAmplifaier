#ifndef FORM_H
#define FORM_H

#include <QWidget>

namespace Ui {
class Form;
class MechPanel;
}

class Form : public QWidget
{
  Q_OBJECT

public:
  explicit Form(QString name, QString ip, quint16 port, QWidget *parent = nullptr);
  ~Form();

  void addMechpanel(Ui::MechPanel* panel);

signals:
  void setFreq(float freq);
  void tuneMode(bool isTuneMode); //true enable tune; false disable tune mode

private slots:
  void on_tuneButton_clicked();
  void on_pushButton_clicked();
  void showEvent(QShowEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

private:
  Ui::Form *ui;
  bool m_tuneMode = false;

  QString m_ip;
  quint16 m_port;
  QString m_name;
  class SocketWrapper* m_client;

};

#endif // FORM_H
