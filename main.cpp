#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QStandardPaths>

#include "LoginDialog.hpp"
#include "database.hpp"
#include "mainwindow.hpp"

// ─────────────────────────────────────────────────────────────────────────────
//  Connection option loaders
// ─────────────────────────────────────────────────────────────────────────────

static QString sqlitePath(const QString& dbName) {
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QDir::separator() + dbName;
}

static PostgresOptions loadPostgresOptions() {
    QByteArray dbName = qgetenv("PGDATABASE");
    QByteArray host = qgetenv("PGHOST");
    QByteArray user = qgetenv("PGUSER");
    QByteArray password = qgetenv("PGPASSWORD");
    QByteArray port = qgetenv("PGPORT");

    if (dbName.isEmpty()) {
        throw std::runtime_error("PGDATABASE environment variable is not set");
    }
    if (user.isEmpty()) {
        throw std::runtime_error("PGUSER environment variable is not set");
    }
    if (password.isEmpty()) {
        throw std::runtime_error("PGPASSWORD environment variable is not set");
    }

    if (host.isEmpty()) {
        host = "127.0.0.1";
    }

    // BUG FIX: original code checked port.isEmpty() instead of !port.isEmpty()
    int portInt = 5432;
    if (!port.isEmpty()) {
        bool ok;
        int p = port.toInt(&ok);
        if (ok) {
            portInt = p;
        }
    }

    return {dbName, user, password, host, portInt};
}

static MysqlOptions loadMysqlOptions() {
    QByteArray dbName = qgetenv("MYSQL_DATABASE");
    QByteArray host = qgetenv("MYSQL_HOST");
    QByteArray user = qgetenv("MYSQL_USER");
    QByteArray password = qgetenv("MYSQL_PASSWORD");
    QByteArray port = qgetenv("MYSQL_PORT");

    if (dbName.isEmpty()) {
        throw std::runtime_error("MYSQL_DATABASE environment variable is not set");
    }
    if (user.isEmpty()) {
        throw std::runtime_error("MYSQL_USER environment variable is not set");
    }
    if (password.isEmpty()) {
        throw std::runtime_error("MYSQL_PASSWORD environment variable is not set");
    }

    if (host.isEmpty()) {
        host = "127.0.0.1";
    }

    int portInt = 3306;
    if (!port.isEmpty()) {
        bool ok;
        int p = port.toInt(&ok);
        if (ok) {
            portInt = p;
        }
    }

    return {dbName, user, password, host, portInt};
}

static ConnOptions loadConnOptions() {
    const QByteArray driver = qgetenv("HMIS_DB_DRIVER");

    if (driver.isEmpty() || driver == "sqlite3") {
        return ConnOptions(SqliteOptions(sqlitePath("hmis.sqlite3")));
    }
    if (driver == "postgresql") {
        return ConnOptions(loadPostgresOptions());
    }
    if (driver == "mysql") {
        return ConnOptions(loadMysqlOptions());
    }
    throw std::runtime_error("Unknown HMIS_DB_DRIVER: " + driver.toStdString());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Palette
// ─────────────────────────────────────────────────────────────────────────────

static void setPalette(QApplication& app) {
    QPalette p;
    p.setColor(QPalette::Window, QColor(240, 240, 240));
    p.setColor(QPalette::WindowText, Qt::black);
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
    p.setColor(QPalette::ToolTipBase, Qt::white);
    p.setColor(QPalette::ToolTipText, Qt::black);
    p.setColor(QPalette::Text, Qt::black);
    p.setColor(QPalette::Button, QColor(240, 240, 240));
    p.setColor(QPalette::ButtonText, Qt::black);
    p.setColor(QPalette::Link, QColor(0, 120, 215));
    p.setColor(QPalette::Highlight, QColor(0, 120, 215));
    p.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(p);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Entry point
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("HMIS");
    app.setApplicationVersion("2.3.0");
    app.setOrganizationName("Yo Medical Files (U) Limited");
    app.setOrganizationDomain("yomedicalfiles.com");
    app.setApplicationDisplayName("HMIS 105");
    app.setQuitOnLastWindowClosed(true);
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);

    Q_INIT_RESOURCE(Resources);
    app.setStyle("Fusion");
    setPalette(app);

    // ── Database ──────────────────────────────────────────────────
    Database db;
    try {
        db.Connect(loadConnOptions());
        db.createSchema();
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Database Error", e.what());
        return EXIT_FAILURE;
    }

    // ── CLI: Create superuser ────────────────────────────────────
    QStringList args = QCoreApplication::arguments();
    if (args.contains("--create-superuser")) {
        QTextStream qin(stdin), qout(stdout);
        qout << "Create superuser (admin)\n";
        qout << "Username: ";
        qout.flush();
        QString username = qin.readLine().trimmed();
        if (username.isEmpty()) {
            qout << "Username cannot be empty.\n";
            return EXIT_FAILURE;
        }
        if (db.userExists(username)) {
            qout << "User already exists.\n";
            return EXIT_FAILURE;
        }
        qout << "Password: ";
        qout.flush();
        QString password = qin.readLine();
        if (password.isEmpty()) {
            qout << "Password cannot be empty.\n";
            return EXIT_FAILURE;
        }
        if (db.createUser(username, password, UserRole::Admin)) {
            qout << "Superuser created successfully.\n";
            return EXIT_SUCCESS;
        }
        qout << "Failed to create superuser.\n";
        return EXIT_FAILURE;
    }

    // ── Login ─────────────────────────────────────────────────────
    LoginDialog login(db);
    if (login.exec() != QDialog::Accepted) {
        return EXIT_SUCCESS;  // User closed the dialog
    }

    auto user = login.authenticatedUser();
    if (!user) {
        return EXIT_SUCCESS;
    }

    // ── Main window ───────────────────────────────────────────────
    MainWindow window(db, *user);
    window.showMaximized();
    return app.exec();
}
