#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QStandardPaths>
#include "mainwindow.hpp"

static const QString APP_NAME      = "HMIS 105";
static const QString APP_VERSION   = "1.0.0";
static const QString APP_AUTHOR    = "Dr. Abiira Nathan";
static const QString APP_COPYRIGHT = "2023 Dr. Abiira Nathan";
static const QString APP_LICENSE   = "MIT License";

static QString SqlitePath(const QString& dbName) {
    // create in home directory
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString dbPath  = homeDir + QDir::separator() + dbName;
    return dbPath;
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(Resources);
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

    // Create connection options
    // You can change the database options here
    ConnOptions connOptions;
    connOptions.setSqliteOptions(SqliteOptions(SqlitePath("hmis.sqlite3")));

    Database db;
    if (!db.Connect(connOptions)) {
        QMessageBox::critical(
            nullptr,
            APP_NAME,
            "Unable to connect to the database. Please check your connection settings." + db.getLastError());
        return EXIT_FAILURE;
    }

    if (!db.createSchema()) {
        QMessageBox::critical(nullptr, APP_NAME, "Unable to create database schema." + db.getLastError());
        return EXIT_FAILURE;
    }

    MainWindow window(db);

    window.showMaximized();
    return app.exec();
}
