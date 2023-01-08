#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  Q_INIT_RESOURCE(Resources);

  a.setStyle("Fusion");
  MainWindow w;

  if (w.dbOpen()) {
    w.showMaximized();
    return a.exec();
  }

  return 1;
}
