#include <QApplication>
#include <QFile>

#include "mainwindow.h"

void messageHandler(QtMsgType type, const QMessageLogContext &context,
                    const QString &msg) {
  QString txt;
  switch (type) {
    case QtDebugMsg:
      txt = QString("Debug: %1").arg(msg);
      break;
    case QtInfoMsg:
      txt = QString("Info: %1").arg(msg);
      break;
    case QtCriticalMsg:
      txt = QString("Critical: %1").arg(msg);
      break;
    case QtWarningMsg:
      txt = QString("Warning: %1").arg(msg);
      break;
    case QtFatalMsg:
      txt = QString("Fatal: %1").arg(msg);
      break;
  }

  QFile outFile("debug.log");
  outFile.open(QIODevice::WriteOnly | QIODevice::Append);
  QTextStream ts(&outFile);
  ts << txt << Qt::endl;
}

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  Q_INIT_RESOURCE(Resources);
  qInstallMessageHandler(messageHandler);

  a.setStyle("Fusion");
  MainWindow w;

  if (w.dbOpen()) {
    w.showMaximized();
    return a.exec();
  }

  return 1;
}
