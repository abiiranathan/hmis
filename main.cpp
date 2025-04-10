#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QStandardPaths>
#include "mainwindow.hpp"

static QString SqlitePath(const QString& dbName) {
    // create in home directory
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString dbPath  = homeDir + QDir::separator() + dbName;
    return dbPath;
}

static void setPallete(QApplication& app) {
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
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("HMIS");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Yo Medical Files (U) Limited");
    app.setOrganizationDomain("yomedicalfiles.com");
    app.setApplicationDisplayName("HMIS");
    app.setQuitOnLastWindowClosed(true);

    // Initialize resources
    Q_INIT_RESOURCE(Resources);

    // Set application style
    app.setStyle("Fusion");

    // Set application palette
    setPallete(app);

    // Create connection options
    // You can change the database options here
    ConnOptions connOptions;
    connOptions.setSqliteOptions(SqliteOptions(SqlitePath("hmis.sqlite3")));

    Database db;
    if (!db.Connect(connOptions)) {
        QMessageBox::critical(
            nullptr,
            "HMIS",
            "Unable to connect to the database. Please check your connection settings." + db.getLastError());
        return EXIT_FAILURE;
    }

    if (!db.createSchema()) {
        QMessageBox::critical(nullptr, "HMIS", "Unable to create database schema." + db.getLastError());
        return EXIT_FAILURE;
    }

    MainWindow window(db);

    window.showMaximized();
    return app.exec();
}
