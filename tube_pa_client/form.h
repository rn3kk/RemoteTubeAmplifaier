#ifndef FORM_H
#define FORM_H


#include <QMap>
#include <QWidget>
#include "../common/model.h"

namespace Ui {
class Form;
class MechPanel;
}

class Form : public QWidget
{
  Q_OBJECT

public:
  explicit Form(QString name, QString ip, QString port, QWidget *parent = nullptr);
  ~Form();

  void setPwrState(bool state);
  void setTuneMode(bool state);
  void setMechaduinoParams(int mech1_pos, int mech2_pos);
  void setFreq(float freq);
  void setProtectionStatus(int status);

signals:
  void tuneMode();
  void pwr();
  void mechChanged(const Mechaduino&);
  void relay(int relay);

private slots:
  void on_tuneButton_clicked();
  void on_pwrButton_clicked();
  void closeEvent(QCloseEvent *event) override;

private:
  Ui::Form *ui;

  class  QLineEdit* m_freqEdit;
  class QPushButton* m_pwrButton;  
  class MechPanel* m_mech_panel1; // tune
  MechPanel* m_mech_panel2; //load
  bool m_tuneMode=false;

};

#endif // FORM_H
