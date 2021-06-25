#ifndef FORM_H
#define FORM_H


#include <QMap>
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

  void setPwrState(bool state);
  void setTuneMode(bool state);
  void changeMechPanel(QString name, int pos);
  void setFreq(float freq);

signals:
  void sendRequest(const QByteArray&);
  void tuneMode(bool isTuneMode); //true enable tune; false disable tune mode

private slots:
  void on_tuneButton_clicked();
  void on_pwrButton_clicked();  
  void needChangeMechPos(int pos);  
  void closeEvent(QCloseEvent *event) override;

private:
  Ui::Form *ui;
  bool m_tuneMode = false;

  class  QLineEdit* m_freqEdit;
  class QPushButton* m_pwrButton;
  QMap<QString, class MechPanel*> m_mechPanels;
  bool m_pwr = false;

};

#endif // FORM_H
