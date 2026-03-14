#include "database.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QtSql/QSqlRecord>
#include <exception>
#include <utility>

// ---------------------------------------------------------------------------
// RAII transaction guard (same pattern as original)
// ---------------------------------------------------------------------------
struct TransactionGuard {
    QSqlDatabase& db;
    bool active;
    explicit TransactionGuard(QSqlDatabase& d) : db(d), active(db.transaction()) {}
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
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
Database::Database() = default;

// ---------------------------------------------------------------------------
// Connection
// ---------------------------------------------------------------------------
void Database::Connect(const ConnOptions& options) {
    if (!options.isValid()) {
        throw std::runtime_error("Invalid connection options");
    }

    const QString driverName = options.getDriverName();
    if (!QSqlDatabase::isDriverAvailable(driverName)) {
        throw std::runtime_error("Unsupported driver: " + driverName.toStdString());
    }

    db = QSqlDatabase::addDatabase(driverName);

    switch (options.getDriver()) {
        case Driver::SQLITE: {
            const auto& opt = options.get<SqliteOptions>();
            db.setDatabaseName(opt.dbName);
            break;
        }
        case Driver::POSTGRES: {
            const auto& opt = options.get<PostgresOptions>();
            db.setHostName(opt.getHost());
            db.setPort(opt.getPort());
            db.setDatabaseName(opt.getDbName());
            db.setUserName(opt.getUser());
            db.setPassword(opt.getPassword());
            break;
        }
        case Driver::MYSQL: {
            const auto& opt = options.get<MysqlOptions>();
            db.setHostName(opt.getHost());
            db.setPort(opt.getPort());
            db.setDatabaseName(opt.getDbName());
            db.setUserName(opt.getUser());
            db.setPassword(opt.getPassword());
            break;
        }
    }

    if (!db.open()) {
        throw std::runtime_error("Database connection failed: " + db.lastError().text().toStdString());
    }

    m_connOptions = options;

    // Enable WAL mode for SQLite (better concurrency + crash safety)
    if (options.getDriver() == Driver::SQLITE) {
        QSqlQuery q;
        q.exec("PRAGMA journal_mode=WAL");
        q.exec("PRAGMA foreign_keys=ON");
    }
}

// ---------------------------------------------------------------------------
// Schema creation
// ---------------------------------------------------------------------------
void Database::createSchema() {
    Driver driver = m_connOptions.getDriver();
    QString pkDef;

    if (driver == Driver::SQLITE) {
        pkDef = "id integer NOT NULL PRIMARY KEY AUTOINCREMENT";
    } else if (driver == Driver::POSTGRES) {
        pkDef = "id SERIAL PRIMARY KEY";
    } else if (driver == Driver::MYSQL) {
        pkDef = "id INT NOT NULL AUTO_INCREMENT PRIMARY KEY";
    } else {
        throw std::runtime_error("Unsupported database driver");
    }

    QSqlQuery q;

    // Main HMIS table
    if (!q.exec("CREATE TABLE IF NOT EXISTS hmis (" + pkDef +
                ","
                "age_category TEXT NOT NULL,"
                "month INT NOT NULL,"
                "year INT NOT NULL,"
                "sex TEXT CHECK(sex IN ('Male','Female')) NOT NULL,"
                "new_attendance TEXT CHECK(new_attendance IN ('YES','NO')) NOT NULL DEFAULT 'YES',"
                "diagnosis TEXT DEFAULT '',"
                "ip_number VARCHAR(100) NOT NULL,"
                "UNIQUE(ip_number, year, month))")) {
        throw std::runtime_error("Error creating hmis table: " + q.lastError().text().toStdString());
    }

    // Diagnoses lookup table
    if (!q.exec("CREATE TABLE IF NOT EXISTS diagnoses (" + pkDef +
                ","
                "name TEXT NOT NULL UNIQUE)")) {
        throw std::runtime_error("Error creating diagnoses table: " + q.lastError().text().toStdString());
    }

    // Users table
    if (!q.exec("CREATE TABLE IF NOT EXISTS users (" + pkDef +
                ","
                "username TEXT NOT NULL UNIQUE,"
                "password_hash TEXT NOT NULL,"
                "salt TEXT NOT NULL,"
                "role TEXT CHECK(role IN ('Admin','Clerk')) NOT NULL DEFAULT 'Clerk')")) {
        throw std::runtime_error("Error creating users table: " + q.lastError().text().toStdString());
    }

    // Audit log table
    if (!q.exec("CREATE TABLE IF NOT EXISTS audit_log (" + pkDef +
                ","
                "user_id INT NOT NULL DEFAULT 0,"
                "username TEXT NOT NULL DEFAULT '',"
                "action TEXT NOT NULL,"
                "table_name TEXT NOT NULL DEFAULT 'hmis',"
                "record_id INT NOT NULL DEFAULT 0,"
                "detail TEXT NOT NULL DEFAULT '',"
                "changed_at TEXT NOT NULL)")) {
        throw std::runtime_error("Error creating audit_log table: " + q.lastError().text().toStdString());
    }
}

// ---------------------------------------------------------------------------
// Error
// ---------------------------------------------------------------------------
QString Database::getLastError() const { return db.lastError().text(); }

// ---------------------------------------------------------------------------
// HMIS data
// ---------------------------------------------------------------------------
HMISData Database::fetchHMISData(int year, int month) {
    QSqlQuery query;
    query.prepare("SELECT * FROM hmis WHERE year=:year AND month=:month ORDER BY id ASC");
    query.bindValue(":year", year);
    query.bindValue(":month", month);

    HMISData rows;
    if (query.exec()) {
        while (query.next()) {
            HMISRow row;
            row.id = query.value(0).toInt();
            row.ageCategory = query.value(1).toString();
            row.month = query.value(2).toInt();
            row.year = query.value(3).toInt();
            row.sex = query.value(4).toString();
            row.newAttendance = query.value(5).toString();
            row.diagnoses = query.value(6).toString().split(dxSeparator, Qt::SkipEmptyParts);
            row.ipNumber = query.value(7).toString();
            rows << row;
        }
    }
    return rows;
}

bool Database::saveNewRow(const NewHMISData& data, int actorUserId) {
    // Duplicate check
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM hmis WHERE ip_number=:ip AND month=:month AND year=:year");
    checkQuery.bindValue(":ip", data.ipNumber);
    checkQuery.bindValue(":month", data.month);
    checkQuery.bindValue(":year", data.year);

    if (!checkQuery.exec()) {
        qWarning() << "Duplicate check failed:" << checkQuery.lastError().text();
        return false;
    }
    if (checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        QMessageBox::warning(nullptr, "Duplicate IP Number", "IP number already exists for the given month and year.");
        return false;
    }

    TransactionGuard guard(db);
    if (!guard.active) {
        qWarning() << "saveNewRow: failed to start transaction";
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "INSERT INTO hmis (age_category, month, year, sex, new_attendance, diagnosis, ip_number) "
        "VALUES(:age_category, :month, :year, :sex, :new_attendance, :diagnosis, :ip_number)");
    query.bindValue(":age_category", data.ageCategory);
    query.bindValue(":month", data.month);
    query.bindValue(":year", data.year);
    query.bindValue(":sex", data.sex);
    query.bindValue(":new_attendance", data.newAttendance);
    query.bindValue(":diagnosis", data.diagnoses.join(dxSeparator));
    query.bindValue(":ip_number", data.ipNumber);

    if (!query.exec()) {
        qWarning() << "saveNewRow insert failed:" << query.lastError().text();
        return false;
    }

    int newId = query.lastInsertId().toInt();
    logAudit(query, actorUserId, "INSERT", "hmis", newId,
             QString("ip=%1 month=%2/%3").arg(data.ipNumber).arg(data.month).arg(data.year));

    return guard.commit();
}

bool Database::updateHMISRow(const HMISRow& data, int actorUserId) {
    TransactionGuard guard(db);
    if (!guard.active) {
        qWarning() << "updateHMISRow: failed to start transaction";
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "UPDATE hmis SET ip_number=:ip, new_attendance=:att, sex=:sex, "
        "age_category=:age, diagnosis=:dx WHERE id=:id");
    query.bindValue(":ip", data.ipNumber);
    query.bindValue(":att", data.newAttendance);
    query.bindValue(":sex", data.sex);
    query.bindValue(":age", data.ageCategory);
    query.bindValue(":dx", data.diagnoses.join(dxSeparator));
    query.bindValue(":id", data.id);

    if (!query.exec()) {
        qWarning() << "updateHMISRow failed:" << query.lastError().text();
        return false;
    }

    logAudit(query, actorUserId, "UPDATE", "hmis", data.id, QString("ip=%1").arg(data.ipNumber));

    return guard.commit();
}

bool Database::deleteHMISRow(int id, int actorUserId) {
    TransactionGuard guard(db);
    if (!guard.active) {
        qWarning() << "deleteHMISRow: failed to start transaction";
        return false;
    }

    QSqlQuery query;
    query.prepare("DELETE FROM hmis WHERE id=:id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "deleteHMISRow failed:" << query.lastError().text();
        return false;
    }

    logAudit(query, actorUserId, "DELETE", "hmis", id, "");
    return guard.commit();
}

QString Database::nextIPNumber(int year, int month) {
    QSqlQuery query;
    query.prepare("SELECT ip_number FROM hmis WHERE year=:year AND month=:month ORDER BY id DESC LIMIT 1");
    query.bindValue(":year", year);
    query.bindValue(":month", month);

    if (query.exec() && query.next()) {
        QString ipNumber = query.value(0).toString();
        if (!ipNumber.isEmpty()) {
            bool ok;
            int n = ipNumber.toInt(&ok);
            if (ok) {
                return QString("%1").arg(n + 1, 3, 10, QChar('0'));
            }
        }
    }
    return "001";
}

// ---------------------------------------------------------------------------
// Diagnoses
// ---------------------------------------------------------------------------
std::optional<QList<Diagnosis>> Database::getAllDiagnoses() {
    QList<Diagnosis> list;
    QSqlQuery query;
    if (!query.exec("SELECT id, name FROM diagnoses ORDER BY name ASC")) {
        qWarning() << "getAllDiagnoses failed:" << query.lastError();
        return std::nullopt;
    }
    while (query.next()) {
        list << Diagnosis{.id = query.value("id").toInt(), .name = query.value("name").toString()};
    }
    return list;
}

bool Database::insertDiagnoses(const QStringList& diagnoses) {
    if (diagnoses.isEmpty()) {
        return true;
    }

    TransactionGuard guard(db);
    if (!guard.active) {
        qWarning() << "insertDiagnoses: failed to start transaction";
        return false;
    }

    QSqlQuery query;
    if (!query.prepare("INSERT INTO diagnoses(name) VALUES(:name)")) {
        qWarning() << "insertDiagnoses prepare failed:" << query.lastError();
        return false;
    }

    for (const QString& name : diagnoses) {
        query.bindValue(":name", name);
        if (!query.exec()) {
            qWarning() << "insertDiagnoses exec failed:" << query.lastError();
            return false;
        }
    }
    return guard.commit();
}

bool Database::diagnosisExists(const QString& name) {
    QSqlQuery query;
    if (!query.prepare("SELECT EXISTS(SELECT 1 FROM diagnoses WHERE name=:name LIMIT 1)")) {
        return false;
    }
    query.bindValue(":name", name);
    if (!query.exec()) {
        return false;
    }
    return query.next() && query.value(0).toBool();
}

// ---------------------------------------------------------------------------
// Authentication helpers
// ---------------------------------------------------------------------------
QString Database::generateSalt() {
    QByteArray bytes(16, Qt::Uninitialized);
    for (auto& b : bytes) {
        b = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }
    return bytes.toHex();
}

QString Database::hashPassword(const QString& password, const QString& salt) {
    QByteArray data = (password + salt).toUtf8();
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
}

// ---------------------------------------------------------------------------
// Users
// ---------------------------------------------------------------------------
bool Database::userExists(const QString& username) {
    QSqlQuery q;
    q.prepare("SELECT EXISTS(SELECT 1 FROM users WHERE username=:u LIMIT 1)");
    q.bindValue(":u", username);
    return q.exec() && q.next() && q.value(0).toBool();
}

bool Database::createUser(const QString& username, const QString& password, UserRole role) {
    QString salt = generateSalt();
    QString hash = hashPassword(password, salt);
    QString roleStr = (role == UserRole::Admin) ? "Admin" : "Clerk";

    QSqlQuery q;
    q.prepare("INSERT INTO users(username, password_hash, salt, role) VALUES(:u, :h, :s, :r)");
    q.bindValue(":u", username);
    q.bindValue(":h", hash);
    q.bindValue(":s", salt);
    q.bindValue(":r", roleStr);

    if (!q.exec()) {
        qWarning() << "createUser failed:" << q.lastError();
        return false;
    }
    return true;
}

std::optional<User> Database::authenticate(const QString& username, const QString& password) {
    QSqlQuery q;
    q.prepare("SELECT id, password_hash, salt, role FROM users WHERE username=:u LIMIT 1");
    q.bindValue(":u", username);

    if (!q.exec() || !q.next()) {
        return std::nullopt;
    }

    int id = q.value(0).toInt();
    QString storedHash = q.value(1).toString();
    QString salt = q.value(2).toString();
    QString roleStr = q.value(3).toString();

    if (hashPassword(password, salt) != storedHash) {
        return std::nullopt;
    }

    UserRole role = (roleStr == "Admin") ? UserRole::Admin : UserRole::Clerk;
    return User{.id = id, .username = username, .role = role};
}

bool Database::changePassword(int userId, const QString& newPassword) {
    QString salt = generateSalt();
    QString hash = hashPassword(newPassword, salt);

    QSqlQuery q;
    q.prepare("UPDATE users SET password_hash=:h, salt=:s WHERE id=:id");
    q.bindValue(":h", hash);
    q.bindValue(":s", salt);
    q.bindValue(":id", userId);
    return q.exec();
}

QList<User> Database::getAllUsers() {
    QList<User> users;
    QSqlQuery q;
    if (!q.exec("SELECT id, username, role FROM users ORDER BY username ASC")) {
        return users;
    }
    while (q.next()) {
        QString r = q.value(2).toString();
        users << User{.id = q.value(0).toInt(),
                      .username = q.value(1).toString(),
                      .role = (r == "Admin") ? UserRole::Admin : UserRole::Clerk};
    }
    return users;
}

// ---------------------------------------------------------------------------
// Audit log
// ---------------------------------------------------------------------------
void Database::logAudit(QSqlQuery& /*q*/, int actorUserId, const QString& action, const QString& table, int recordId,
                        const QString& detail) {
    // Resolve username from id
    QString username;
    if (actorUserId > 0) {
        QSqlQuery uq;
        uq.prepare("SELECT username FROM users WHERE id=:id");
        uq.bindValue(":id", actorUserId);
        if (uq.exec() && uq.next()) {
            username = uq.value(0).toString();
        }
    }
    if (username.isEmpty()) {
        username = "system";
    }

    QSqlQuery aq;
    aq.prepare(
        "INSERT INTO audit_log(user_id, username, action, table_name, record_id, detail, changed_at) "
        "VALUES(:uid, :u, :a, :t, :rid, :d, :ts)");
    aq.bindValue(":uid", actorUserId);
    aq.bindValue(":u", username);
    aq.bindValue(":a", action);
    aq.bindValue(":t", table);
    aq.bindValue(":rid", recordId);
    aq.bindValue(":d", detail);
    aq.bindValue(":ts", QDateTime::currentDateTime().toString(Qt::ISODate));
    aq.exec();  // best-effort — don't fail the parent operation if audit fails
}

QList<AuditEntry> Database::getAuditLog(int limit) {
    QList<AuditEntry> entries;
    QSqlQuery q;
    q.prepare(
        "SELECT id, username, action, table_name, record_id, detail, changed_at "
        "FROM audit_log ORDER BY id DESC LIMIT :lim");
    q.bindValue(":lim", limit);
    if (!q.exec()) {
        return entries;
    }

    while (q.next()) {
        entries << AuditEntry{.id = q.value(0).toInt(),
                              .username = q.value(1).toString(),
                              .action = q.value(2).toString(),
                              .tableName = q.value(3).toString(),
                              .recordId = q.value(4).toInt(),
                              .detail = q.value(5).toString(),
                              .changedAt = q.value(6).toString()};
    }
    return entries;
}

// ---------------------------------------------------------------------------
// Stats helpers
// ---------------------------------------------------------------------------
MonthlyStats Database::buildAttendanceStats(const HMISData& rows) const {
    MonthlyStats stats;
    for (const HMISRow& row : rows) {
        stats.increment(row.newAttendance, row.ageCategory, row.sex);
    }
    return stats;
}

MonthlyStats Database::buildDiagnosisStats(const HMISData& rows, const QStringList& diagnosisNames) const {
    MonthlyStats stats;
    // Pre-seed keys so zero-count diagnoses still exist
    for (const QString& dx : diagnosisNames) {
        for (const QString& age : AGE_CATEGORIES) {
            stats.data[dx][age];  // default-constructs CategoryCount{0,0}
        }
    }

    for (const HMISRow& row : rows) {
        for (const QString& dx : row.diagnoses) {
            stats.increment(dx, row.ageCategory, row.sex);
        }
    }
    return stats;
}

// ---------------------------------------------------------------------------
// Monthly summary for dashboard
// ---------------------------------------------------------------------------
Database::MonthlySummary Database::getMonthlySummary(int year, int month) {
    HMISData rows = fetchHMISData(year, month);
    MonthlySummary s;
    s.totalPatients = static_cast<int>(rows.size());

    QHash<QString, int> dxCount;
    for (const HMISRow& row : rows) {
        if (row.newAttendance == ATT_YES) {
            s.newAttendances++;
        } else {
            s.reAttendances++;
        }
        for (const QString& dx : row.diagnoses) {
            dxCount[dx]++;
        }
    }

    // Top 3 diagnoses
    QList<QPair<int, QString>> sorted;
    for (auto it = dxCount.begin(); it != dxCount.end(); ++it) {
        sorted << qMakePair(it.value(), it.key());
    }

    std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return a.first > b.first; });
    if (static_cast<int>(sorted.size()) > 0) {
        s.topDiagnosis1 = sorted[0].second;
    }
    if (static_cast<int>(sorted.size()) > 1) {
        s.topDiagnosis2 = sorted[1].second;
    }
    if (static_cast<int>(sorted.size()) > 2) {
        s.topDiagnosis3 = sorted[2].second;
    }

    return s;
}

// ---------------------------------------------------------------------------
// CSV export
// ---------------------------------------------------------------------------
QString Database::exportCSV(int year, int month) {
    HMISData rows = fetchHMISData(year, month);
    QString csv;
    csv += "ID,IP Number,Age Category,Sex,New Attendance,Diagnoses\n";
    for (const HMISRow& row : rows) {
        QString dxCombined = row.diagnoses.join("; ").replace(",", " ");
        csv += QString("%1,%2,\"%3\",%4,%5,\"%6\"\n")
                   .arg(row.id)
                   .arg(row.ipNumber)
                   .arg(row.ageCategory)
                   .arg(row.sex)
                   .arg(row.newAttendance)
                   .arg(dxCombined);
    }
    return csv;
}

// ---------------------------------------------------------------------------
// SQLite backup
// ---------------------------------------------------------------------------
bool Database::backupTo(const QString& destPath) {
    if (m_connOptions.getDriver() != Driver::SQLITE) {
        qWarning() << "Backup only supported for SQLite";
        return false;
    }
    QString srcPath = m_connOptions.dbFilePath();
    if (srcPath.isEmpty() || srcPath == ":memory:") {
        return false;
    }

    if (QFile::exists(destPath)) {
        QFile::remove(destPath);
    }

    return QFile::copy(srcPath, destPath);
}
