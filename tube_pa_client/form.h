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
  void sendRequest(const QByteArray&);
  void setFreq(float freq);
  void tuneMode(bool isTuneMode); //true enable tune; false disable tune mode

private slots:
  void on_tuneButton_clicked();
  void on_pwrButton_clicked();
  void remoteModelIsChanged(const QByteArray& data);
  void showEvent(QShowEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

private:
  void setPwrState(bool state);

private:
  Ui::Form *ui;
  bool m_tuneMode = false;

  QString m_ip;
  quint16 m_port;
  QString m_name;
  class SocketWrapper* m_client;

  class  QLineEdit* m_lineEdit;
  class QPushButton* m_pwrButton;
  bool m_pwr = false;

};

#endif // FORM_H
