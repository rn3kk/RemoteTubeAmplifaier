#ifndef AMPITEMWIDGET_H
#define AMPITEMWIDGET_H

#include <QWidget>
#include <QDateTime>

namespace Ui {
class AmpItemWidget;
}

class AmpItemWidget : public QWidget
{
  Q_OBJECT

public:
  explicit AmpItemWidget(QString ip, quint16 port, QString paName, QWidget *parent = nullptr);
  ~AmpItemWidget();

  QString ip() const;
  quint16 port() const;
  QString name() const;
  QDateTime updateTime() const;
  void setNewTime(); // when receive last broadcas signal from amp
  bool operator==(AmpItemWidget* w);

private slots:
  void on_connectButton_clicked();

private:
  Ui::AmpItemWidget *ui;
  QString m_ip;
  quint16 m_port;
  QString m_name;
  QDateTime m_updateTime;  

};

#endif // AMPITEMWIDGET_H
