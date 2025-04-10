#include "database.hpp"
#include <qmessagebox.h>

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

Database::Database() = default;

bool Database::Connect(const ConnOptions& options) {
    if (!options.isValid()) {
        qWarning() << "Invalid connection options provided.";
        return false;
    }

    const QString driverName = options.getDriverName();
    if (!QSqlDatabase::isDriverAvailable(driverName)) {
        qWarning() << "Driver" << driverName << "is not available.";
        return false;
    }

    // Set the database name and connection name
    db = QSqlDatabase::addDatabase(driverName);

    switch (options.getDriver()) {
        case Driver::SQLITE: {
            db.setDatabaseName(options.getSqliteOptions().dbName);
        } break;
        case Driver::POSTGRES: {
            const PostgresOptions& postgresOptions = options.getPostgresOptions();
            db.setHostName(postgresOptions.getHost());
            db.setPort(postgresOptions.getPort());
            db.setDatabaseName(postgresOptions.getDbName());
            db.setUserName(postgresOptions.getUser());
            db.setPassword(postgresOptions.getPassword());
        } break;
        case Driver::MYSQL: {
            const MysqlOptions& mysqlOptions = options.getMysqlOptions();
            db.setHostName(mysqlOptions.getHost());
            db.setPort(mysqlOptions.getPort());
            db.setDatabaseName(mysqlOptions.getDbName());
            db.setUserName(mysqlOptions.getUser());
            db.setPassword(mysqlOptions.getPassword());
        } break;
        default:
            qWarning() << "Unsupported database driver.";
            return false;
    }

    bool ok = db.open();
    if (!ok) {
        qWarning() << "Database connection failed:" << db.lastError().text();
        return false;
    }

    qDebug() << "Database connection established successfully with driver" << driverName << "at" << db.databaseName();
    qDebug() << "Database connection name:" << db.connectionName();
    return true;
}

bool Database::createSchema() {
    const QString hmis_schema =
        "CREATE TABLE IF NOT EXISTS hmis ("
        "id integer NOT NULL PRIMARY KEY AUTOINCREMENT, "
        "age_category TEXT NOT NULL,"
        "month int NOT NULL, "
        "year int NOT NULL, "
        "sex TEXT CHECK(sex IN ('Male', 'Female')) NOT NULL, "
        "new_attendance TEXT CHECK(new_attendance IN ('YES', 'NO')) NOT NULL "
        "DEFAULT 'YES', diagnosis TEXT DEFAULT '', ip_number varchar(100) NOT NULL, UNIQUE(ip_number, year, month))";

    QSqlQuery query(hmis_schema);
    if (!query.exec()) {
        qDebug() << "Error executing HMIS schema: " + query.lastError().text() + "\n";
        return false;
    }

    // Create a table for diagnoses
    query = QSqlQuery(
        "CREATE TABLE IF NOT EXISTS diagnoses(id integer NOT NULL PRIMARY KEY AUTOINCREMENT,name TEXT NOT NULL "
        "UNIQUE)");

    if (!query.exec()) {
        qDebug() << "Error executing diagnosis schema: " + query.lastError().text() + "\n";
        return false;
    }
    return true;
}

QString Database::getLastError() {
    return db.lastError().text();
}

HMISData Database::fetchHMISData(int year, int month) {
    QSqlQuery query;

    query.prepare("SELECT * FROM hmis WHERE year=:year AND month=:month");
    query.bindValue(":year", year);
    query.bindValue(":month", month);

    HMISData rows{};

    if (query.exec()) {
        while (query.next()) {
            HMISRow row;
            row.id            = query.value(0).toInt();
            row.ageCategory   = query.value(1).toString();
            row.month         = query.value(2).toInt();
            row.year          = query.value(3).toInt();
            row.sex           = query.value(4).toString();
            row.newAttendance = query.value(5).toString();
            row.diagnoses     = query.value(6).toString().split(dxSeparator);
            row.ipNumber      = query.value(7).toString();
            rows << row;
        }
    }
    return rows;
}

bool Database::saveNewRow(const NewHMISData& data) {
    QSqlQuery checkQuery;
    checkQuery.prepare(
        "SELECT COUNT(*) FROM hmis WHERE ip_number = :ip_number "
        "AND month = :month AND year = :year;");

    checkQuery.bindValue(":ip_number", data.ipNumber);
    checkQuery.bindValue(":month", data.month);
    checkQuery.bindValue(":year", data.year);

    if (!checkQuery.exec()) {
        QMessageBox::warning(
            nullptr, "DB ERROR", "Error checking for duplicate IP number:" + checkQuery.lastError().text());
        return false;
    }

    if (checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        QMessageBox::warning(nullptr, "Duplicate IP Number", "IP number already exists for the given month and year.");
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "INSERT INTO hmis (age_category, month, year, sex, new_attendance, "
        "diagnosis, ip_number) "
        "VALUES(:age_category, :month, :year, :sex, :new_attendance, "
        ":diagnosis, :ip_number);");

    query.bindValue(":age_category", data.ageCategory);
    query.bindValue(":month", data.month);
    query.bindValue(":year", data.year);
    query.bindValue(":sex", data.sex);
    query.bindValue(":new_attendance", data.newAttendance);
    query.bindValue(":diagnosis", data.diagnoses.join(dxSeparator));
    query.bindValue(":ip_number", data.ipNumber);

    if (!query.exec()) {
        qWarning() << "Database error while inserting new row:" << query.lastError().text();
        return false;
    }

    return true;
}

// Latest Ip Number
QString Database::nextIPNumber(int year, int month) {
    QSqlQuery query;
    QString ipNumber;

    query.prepare(
        "SELECT ip_number FROM hmis WHERE year=:year AND month=:month "
        "ORDER BY id DESC LIMIT 1");
    query.bindValue(":year", year);
    query.bindValue(":month", month);

    if (query.exec()) {
        if (query.next()) {
            ipNumber = query.value(0).toString();
            if (!ipNumber.isEmpty()) {
                bool ok;
                int ipNumberInt = ipNumber.toInt(&ok);
                if (ok) {
                    ipNumberInt++;  // Increment
                    // Format with leading zeros
                    ipNumber = QString("%1").arg(ipNumberInt, 3, 10, QChar('0'));
                }
            }
        }
    }
    return ipNumber;
}

bool Database::updateHMISRow(const HMISRow& data) {
    QSqlQuery query;
    query.prepare(
        "UPDATE hmis SET ip_number=:ip_number, new_attendance=:new_attendance, "
        "sex=:sex,"
        "age_category=:age_category, diagnosis=:diagnosis WHERE id=:id");

    query.bindValue(":ip_number", data.ipNumber);
    query.bindValue(":new_attendance", data.newAttendance);
    query.bindValue(":sex", data.sex);
    query.bindValue(":age_category", data.ageCategory);
    query.bindValue(":diagnosis", data.diagnoses.join(dxSeparator));
    query.bindValue(":id", data.id);

    if (!query.exec()) {
        qWarning() << "Database error while updating row:" << query.lastError().text();
        return false;
    }
    return true;
}

// Returns all diagnoses registered in the system.
std::optional<QList<Diagnosis>> Database::getAllDiagnoses() {
    static auto QUERY = QStringLiteral("SELECT id, name FROM diagnoses");
    QList<Diagnosis> list;
    QSqlQuery query(QUERY);

    if (!query.exec()) {
        qWarning() << "Failed to execute getAllDiagnoses query:" << query.lastError();
        return std::nullopt;  // Indicates failure
    }

    list.reserve(query.size());

    while (query.next()) {
        list << Diagnosis{.id = query.value("id").toInt(), .name = query.value("name").toString()};
    }

    return list;
}

bool Database::insertDiagnoses(const QStringList& diagnoses) {
    if (diagnoses.isEmpty()) {
        return true;
    }

    static const auto QUERY = QStringLiteral("INSERT INTO diagnoses(name) VALUES(:name)");
    QSqlQuery query(db);

    // Start transaction with RAII-like scope
    struct TransactionGuard {
        QSqlDatabase& db;
        bool active;
        TransactionGuard(QSqlDatabase& d) : db(d), active(db.transaction()) {}
        ~TransactionGuard() {
            if (active) {
                db.rollback();
            }
        }

        bool commit() {
            if (active) {
                active = false;
                return db.commit();
            }
            return true;
        }
    } guard(db);

    if (!guard.active) {
        qWarning() << "InsertDiagnoses: Failed to start transaction:" << db.lastError();
        return false;
    }

    if (!query.prepare(QUERY)) {
        qWarning() << "Failed to prepare insert diagnoses query:" << query.lastError();
        return false;
    }

    // For each diagnosis in the list, bind the value and execute
    for (const QString& diagnosis : diagnoses) {
        query.bindValue(":name", diagnosis);

        if (!query.exec()) {
            qWarning() << "Insert diagnosis failed:" << query.lastError();
            return false;
        }
    }

    if (!guard.commit()) {
        qWarning() << "InsertDiagnoses: Failed to commit transaction:" << db.lastError();
        return false;  // Treat commit failure as a full failure
    }

    return true;
}

// Check if a diagnosis with the given name exists
bool Database::diagnosisExists(const QString& name) {
    QSqlQuery query;

    if (!query.prepare("SELECT EXISTS (SELECT 1 FROM diagnoses WHERE name = :name LIMIT 1)")) {
        qWarning() << "Failed to prepare diagnosis exists query:" << query.lastError();
        return false;
    }

    query.bindValue(":name", name);

    if (!query.exec()) {
        qWarning() << "Failed to execute diagnosis exists query:" << query.lastError();
        return false;  // Cannot determine existence due to execution error
    }

    // EXISTS query should always return exactly one row (with 0 or 1)
    if (query.next()) {
        return query.value(0).toBool();
    }

    qWarning() << "Diagnosis exists query returned no rows (unexpected).";
    return false;
}

bool Database::deleteHMISRow(int id) {
    QSqlQuery query;
    query.prepare("DELETE FROM hmis WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "Failed to delete HMIS row:" << query.lastError();
        return false;
    }

    return true;
}