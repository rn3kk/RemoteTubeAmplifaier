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
  void setMechaduinoParams(Mechaduino m);
  void setFreq(float freq);

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
  QList<class MechPanel*> m_mechPanels;

};

#endif // FORM_H
