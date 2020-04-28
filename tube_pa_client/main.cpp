#include <QCoreApplication>
#include <QWidget>

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);
  QWidget w;
  w.show();

  return a.exec();
}
