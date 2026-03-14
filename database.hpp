#ifndef DATABASE_H
#define DATABASE_H

#include <QList>
#include <QString>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <optional>
#include <variant>

#include "HMISRow.hpp"
#include "MonthlyStats.hpp"
#include "databaseOptions.hpp"

using HMISData = QList<HMISRow>;

struct NewHMISData {
    QString ageCategory;
    QString sex;
    QString newAttendance;
    QList<QString> diagnoses;
    QString ipNumber;
    int month;
    int year;
};

struct Diagnosis {
    int id;
    QString name;
};

// Role for user accounts
enum class UserRole : uint8_t { Admin, Clerk };

struct User {
    int id;
    QString username;
    UserRole role;
};

struct AuditEntry {
    int id;
    QString username;
    QString action;      // "INSERT" | "UPDATE" | "DELETE"
    QString tableName;
    int recordId;
    QString detail;
    QString changedAt;   // ISO datetime string
};

class Database {
public:
    Database();

    // Connection
    void Connect(const ConnOptions& options);
    void createSchema();
    QString getLastError() const;

    // HMIS data
    HMISData fetchHMISData(int year, int month);
    bool saveNewRow(const NewHMISData& data, int actorUserId = 0);
    bool updateHMISRow(const HMISRow& data, int actorUserId = 0);
    bool deleteHMISRow(int id, int actorUserId = 0);
    QString nextIPNumber(int year, int month);

    // Diagnoses
    std::optional<QList<Diagnosis>> getAllDiagnoses();
    bool insertDiagnoses(const QStringList& diagnoses);
    bool diagnosisExists(const QString& name);

    // Users / auth
    std::optional<User> authenticate(const QString& username, const QString& password);
    bool createUser(const QString& username, const QString& password, UserRole role);
    bool userExists(const QString& username);
    QList<User> getAllUsers();
    bool changePassword(int userId, const QString& newPassword);

    // Audit log
    QList<AuditEntry> getAuditLog(int limit = 500);

    // Export
    // Returns CSV text for the given month/year (attendances + diagnoses)
    QString exportCSV(int year, int month);

    // Backup (SQLite only): copies DB file to destPath
    bool backupTo(const QString& destPath);

    // Stats helpers
    MonthlyStats buildAttendanceStats(const HMISData& rows) const;
    MonthlyStats buildDiagnosisStats(const HMISData& rows, const QStringList& diagnosisNames) const;

    // Summary counts for dashboard
    struct MonthlySummary {
        int totalPatients      = 0;
        int newAttendances     = 0;
        int reAttendances      = 0;
        QString topDiagnosis1;
        QString topDiagnosis2;
        QString topDiagnosis3;
    };
    MonthlySummary getMonthlySummary(int year, int month);

private:
    const QString dxSeparator = "____";

    QSqlDatabase db;
    ConnOptions m_connOptions;

    // Internal helpers
    void logAudit(QSqlQuery& q, int actorUserId, const QString& action,
                  const QString& table, int recordId, const QString& detail);
    static QString hashPassword(const QString& password, const QString& salt);
    static QString generateSalt();
};

// Age categories (shared constants used by UI and DB layer)
inline const QString AGE_0_28D       = "0 - 28 days";
inline const QString AGE_29D_4Y      = "29 days - 4 years";
inline const QString AGE_5_9Y        = "5 - 9 years";
inline const QString AGE_10_19Y      = "10 - 19 years";
inline const QString AGE_20_PLUS     = "20 years and above";
inline const QStringList AGE_CATEGORIES = {
    AGE_0_28D, AGE_29D_4Y, AGE_5_9Y, AGE_10_19Y, AGE_20_PLUS
};

inline const QString SEX_MALE   = "Male";
inline const QString SEX_FEMALE = "Female";
inline const QString ATT_YES    = "YES";
inline const QString ATT_NO     = "NO";

#endif  // DATABASE_H
