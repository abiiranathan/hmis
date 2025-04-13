#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QStandardPaths>
#include "mainwindow.hpp"

static QString SqlitePath(const QString& dbName) {
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString dbPath  = homeDir + QDir::separator() + dbName;
    return dbPath;
}

static PostgresOptions loadPostgresOptions() {
    QByteArray dbName   = qgetenv("PGDATABASE");
    QByteArray host     = qgetenv("PGHOST");
    QByteArray user     = qgetenv("PGUSER");
    QByteArray password = qgetenv("PGPASSWORD");
    QByteArray port     = qgetenv("PGPORT");

    if (host.isEmpty()) {
        host = "127.0.0.1";
    }

    int portInt = 5432;
    if (port.isEmpty()) {
        bool ok;
        int newPort = port.toInt(&ok);
        if (ok) {
            portInt = newPort;
        }
    }

    if (dbName.isEmpty()) {
        throw std::runtime_error("PGDATABASE environment variable is not set");
    }

    if (user.isEmpty()) {
        throw std::runtime_error("PGUSER environment variable is not set");
    }

    if (password.isEmpty()) {
        throw std::runtime_error("PGPASSWORD environment variable is not set");
    }

    return {dbName, user, password, host, portInt};
}

static MysqlOptions loadMysqlOptions() {
    QByteArray dbName   = qgetenv("MYSQL_DATABASE");
    QByteArray host     = qgetenv("MYSQL_HOST");
    QByteArray user     = qgetenv("MYSQL_USER");
    QByteArray password = qgetenv("MYSQL_PASSWORD");
    QByteArray port     = qgetenv("MYSQL_PORT");

    if (host.isEmpty()) {
        host = "127.0.0.1";
    }

    int portInt = 3306;
    if (!port.isEmpty()) {
        bool ok;
        int newPort = port.toInt(&ok);
        if (ok) {
            portInt = newPort;
        }
    }

    if (dbName.isEmpty()) {
        throw std::runtime_error("MYSQL_DATABASE environment variable is not set");
    }

    if (user.isEmpty()) {
        throw std::runtime_error("MYSQL_USER environment variable is not set");
    }

    if (password.isEmpty()) {
        throw std::runtime_error("MYSQL_PASSWORD environment variable is not set");
    }

    return {dbName, user, password, host, portInt};
}

static ConnOptions loadConnOptions() {
    const QByteArray driver = qgetenv("HMIS_DB_DRIVER");
    if (driver.isEmpty() || driver == "sqlite3") {
        return ConnOptions(SqliteOptions(SqlitePath("hmis.sqlite3")));
    }
    if (driver == "postgresql") {
        return ConnOptions(loadPostgresOptions());
    }
    if (driver == "mysql") {
        return ConnOptions(loadMysqlOptions());
    }
    throw std::runtime_error("Unknown driver");
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
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);

    // Initialize resources
    Q_INIT_RESOURCE(Resources);

    // Set application style
    app.setStyle("Fusion");

    // Set application palette
    setPallete(app);

    Database db;

    try {
        // Supported drivers: sqlite3, postgresql, mysql
        db.Connect(loadConnOptions());
        db.createSchema();
    } catch (const std::runtime_error& e) {
        QMessageBox::critical(nullptr, "Database Connection", e.what());
        return EXIT_FAILURE;
    }

    MainWindow window(db);
    window.showMaximized();
    return app.exec();
}
