#ifndef DATABASE_H
#define DATABASE_H

#include <QList>
#include <QString>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <utility>

#include "HMISRow.hpp"

// List of HMIS rows.
using HMISData = QList<HMISRow>;

// Data for creating a new record
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

// Driver enum
enum class Driver : uint8_t { SQLITE, POSTGRES, MYSQL };

// Simple SQLite options
struct SqliteOptions {
    QString dbName;
    SqliteOptions() : dbName("hmis.sqlite3") {}
    SqliteOptions(QString name) : dbName(std::move(name)) {}

    [[nodiscard]] bool isValid() const {
        return !dbName.isEmpty();
    }
};

// Tag types for templated options
struct PostgresTag {};
struct MysqlTag {};

// Templated DatabaseOptions (from your previous question)
template <typename DatabaseTag>
class DatabaseOptions {
public:
    DatabaseOptions() = default;
    DatabaseOptions(QString dbName, QString user, QString password, QString host, int port)
        : m_dbname(std::move(dbName)),
          m_user(std::move(user)),
          m_password(std::move(password)),
          m_host(std::move(host)),
          m_port(port) {}
    [[nodiscard]] const QString& getDbName() const {
        return m_dbname;
    }
    [[nodiscard]] const QString& getUser() const {
        return m_user;
    }
    [[nodiscard]] const QString& getPassword() const {
        return m_password;
    }
    [[nodiscard]] const QString& getHost() const {
        return m_host;
    }
    [[nodiscard]] int getPort() const {
        return m_port;
    }

    [[nodiscard]] bool isValid() const {
        return !m_dbname.isEmpty() && !m_user.isEmpty() && !m_host.isEmpty() && m_port > 0;
    }

    [[nodiscard]] int defaultPort() const {
        if constexpr (std::is_same_v<DatabaseTag, PostgresTag>) {
            return 5432;
        } else if constexpr (std::is_same_v<DatabaseTag, MysqlTag>) {
            return 3306;
        }
        return 0;  // Default for other database types
    }

    [[nodiscard]] QString getConnectionString() {
        return QString("host=%1 port=%2 dbname=%3 user=%4 password=%5")
            .arg(m_host)
            .arg(m_port)
            .arg(m_dbname)
            .arg(m_user)
            .arg(m_password);
    }

private:
    QString m_dbname;
    QString m_user;
    QString m_password;
    QString m_host = "127.0.0.1";
    int m_port     = defaultPort();
};

using PostgresOptions = DatabaseOptions<PostgresTag>;
using MysqlOptions    = DatabaseOptions<MysqlTag>;

class ConnOptions {
public:
    // Constructors
    ConnOptions() : m_driver(Driver::SQLITE) {}

    explicit ConnOptions(Driver driver) : m_driver(driver) {
        switch (driver) {
            case Driver::SQLITE:
                m_sqliteOptions = SqliteOptions();
                break;
            case Driver::POSTGRES:
                m_postgresOptions = PostgresOptions();
                break;
            case Driver::MYSQL:
                m_mysqlOptions = MysqlOptions();
                break;
        }
    }

    explicit ConnOptions(SqliteOptions options) : m_driver(Driver::SQLITE), m_sqliteOptions(std::move(options)) {}
    explicit ConnOptions(PostgresOptions options) : m_driver(Driver::POSTGRES), m_postgresOptions(std::move(options)) {}
    explicit ConnOptions(MysqlOptions options) : m_driver(Driver::MYSQL), m_mysqlOptions(std::move(options)) {}

    // Copy constructor
    ConnOptions(const ConnOptions& other) = default;

    // Move constructor
    ConnOptions(ConnOptions&& other) noexcept
        : m_driver(other.m_driver),
          m_sqliteOptions(std::move(other.m_sqliteOptions)),
          m_postgresOptions(std::move(other.m_postgresOptions)),
          m_mysqlOptions(std::move(other.m_mysqlOptions)) {}

    // Copy assignment
    ConnOptions& operator=(const ConnOptions& other) = default;

    // Move assignment
    ConnOptions& operator=(ConnOptions&& other) noexcept {
        if (this != &other) {
            m_driver          = other.m_driver;
            m_sqliteOptions   = std::move(other.m_sqliteOptions);
            m_postgresOptions = std::move(other.m_postgresOptions);
            m_mysqlOptions    = std::move(other.m_mysqlOptions);
        }
        return *this;
    }

    // Destructor
    ~ConnOptions() = default;

    // Getters
    [[nodiscard]] Driver getDriver() const {
        return m_driver;
    }

    [[nodiscard]] QString getDriverName() const {
        switch (m_driver) {
            case Driver::SQLITE:
                return "QSQLITE";
            case Driver::POSTGRES:
                return "QPSQL";
            case Driver::MYSQL:
                return "QMYSQL";
        }
        return {};
    }

    [[nodiscard]] QString getConnectionString() {
        switch (m_driver) {
            case Driver::SQLITE:
                return m_sqliteOptions.dbName;
            case Driver::POSTGRES:
                return m_postgresOptions.getConnectionString();
            case Driver::MYSQL:
                return m_mysqlOptions.getConnectionString();
        }
        return {};
    }
    [[nodiscard]] const SqliteOptions& getSqliteOptions() const {
        return m_sqliteOptions;
    }
    [[nodiscard]] const PostgresOptions& getPostgresOptions() const {
        return m_postgresOptions;
    }
    [[nodiscard]] const MysqlOptions& getMysqlOptions() const {
        return m_mysqlOptions;
    }

    // Setters
    void setDriver(Driver driver) {
        m_driver = driver;
    }
    void setSqliteOptions(const SqliteOptions& options) {
        m_sqliteOptions = options;
        m_driver        = Driver::SQLITE;
    }
    void setPostgresOptions(const PostgresOptions& options) {
        m_postgresOptions = options;
        m_driver          = Driver::POSTGRES;
    }
    void setMysqlOptions(const MysqlOptions& options) {
        m_mysqlOptions = options;
        m_driver       = Driver::MYSQL;
    }

    [[nodiscard]] bool isValid() const {
        switch (m_driver) {
            case Driver::SQLITE:
                return !m_sqliteOptions.dbName.isEmpty();
            case Driver::POSTGRES:
                return !m_postgresOptions.getDbName().isEmpty() && !m_postgresOptions.getUser().isEmpty() &&
                       !m_postgresOptions.getPassword().isEmpty() && !m_postgresOptions.getHost().isEmpty() &&
                       m_postgresOptions.getPort() > 0;
            case Driver::MYSQL:
                return !m_mysqlOptions.getDbName().isEmpty() && !m_mysqlOptions.getUser().isEmpty() &&
                       !m_mysqlOptions.getPassword().isEmpty() && !m_mysqlOptions.getHost().isEmpty() &&
                       m_mysqlOptions.getPort() > 0;
        }
        return false;
    }

private:
    Driver m_driver;
    SqliteOptions m_sqliteOptions;
    PostgresOptions m_postgresOptions;
    MysqlOptions m_mysqlOptions;
};

class Database {
private:
    const QString dxSeparator = "____";

    // Instance of the database
    QSqlDatabase db;
    ConnOptions connOptions;

public:
    // Default constructor
    Database();

    // Connect to the database with the given options. If the connection fails, returns false.
    bool Connect(const ConnOptions& options);

    // Create the tables. If an error occurs, returns false.
    bool createSchema();

    QString getLastError();

    // Fetch data for a given month and year.
    HMISData fetchHMISData(int year, int month);

    // Save new HMIS row
    bool saveNewRow(const NewHMISData& data);

    // Update hmis row
    bool updateHMISRow(const HMISRow& data);

    // Delete hmis row
    bool deleteHMISRow(int id);

    // Latest Ip Number
    QString nextIPNumber(int year, int month);

    // Working with diagnoses
    // =================================

    // Returns all diagnoses registered in the system.
    std::optional<QList<Diagnosis>> getAllDiagnoses();

    // Insert one or more diagnoses
    bool insertDiagnoses(const QStringList& diagnoses);

    bool diagnosisExists(const QString& name);
};

#endif  // DATABASE_H
