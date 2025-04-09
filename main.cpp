#include <QApplication>
#include <QFile>
#include <iostream>

#include "mainwindow.h"

void messageHandler(QtMsgType type, const QMessageLogContext & /*context*/,
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
  default:
    std::cout << msg.toStdString() << "\n";
  }

  QFile outFile("debug.log");
  outFile.open(QIODevice::WriteOnly | QIODevice::Append);
  QTextStream stream(&outFile);
  stream << txt << "\n";
}

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  Q_INIT_RESOURCE(Resources);
  // qInstallMessageHandler(messageHandler);
  app.setStyle("Fusion");

  // Create light theme palette
  QPalette palette;
  palette.setColor(QPalette::Window, QColor(240, 240, 240));
  palette.setColor(QPalette::WindowText, Qt::black);
  palette.setColor(QPalette::Base, Qt::white);
  palette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
  palette.setColor(QPalette::ToolTipBase, Qt::white);
  palette.setColor(QPalette::ToolTipText, Qt::black);
  palette.setColor(QPalette::Text, Qt::black);
  palette.setColor(QPalette::Button, QColor(240, 240, 240));
  palette.setColor(QPalette::ButtonText, Qt::black);
  palette.setColor(QPalette::Link, QColor(0, 120, 215));
  palette.setColor(QPalette::Highlight, QColor(0, 120, 215));
  palette.setColor(QPalette::HighlightedText, Qt::white);
  app.setPalette(palette);

  MainWindow window;

  if (window.dbOpen()) {
    window.showMaximized();
    return app.exec();
  }

  return EXIT_FAILURE;
}
