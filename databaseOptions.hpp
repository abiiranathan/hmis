#ifndef DATABASEOPTIONS_H
#define DATABASEOPTIONS_H

#include <QString>
#include <type_traits>
#include <variant>

// Enum for driver
enum class Driver : uint8_t { SQLITE, POSTGRES, MYSQL };

// Sqlite options
struct SqliteOptions {
    QString dbName = "hmis.sqlite3";

    SqliteOptions() = default;
    SqliteOptions(QString name) : dbName(std::move(name)) {}

    [[nodiscard]] bool isValid() const {
        return !dbName.isEmpty();
    }
};

// Tag types for templated options
struct PostgresTag {};
struct MysqlTag {};

// Templated options for Postgres/MySQL
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
        }
        if constexpr (std::is_same_v<DatabaseTag, MysqlTag>) {
            return 3306;
        }
        return 0;
    }

    [[nodiscard]] QString getConnectionString() const {
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

template <typename T>
concept AllowedVariant =
    std::is_same_v<T, SqliteOptions> || std::is_same_v<T, PostgresOptions> || std::is_same_v<T, MysqlOptions>;

// Unified ConnOptions using std::variant
class ConnOptions {
public:
    using Variant = std::variant<SqliteOptions, PostgresOptions, MysqlOptions>;

    ConnOptions() : m_driver(Driver::SQLITE), m_options(SqliteOptions()) {}
    explicit ConnOptions(SqliteOptions opt) : m_driver(Driver::SQLITE), m_options(std::move(opt)) {}
    explicit ConnOptions(PostgresOptions opt) : m_driver(Driver::POSTGRES), m_options(std::move(opt)) {}
    explicit ConnOptions(MysqlOptions opt) : m_driver(Driver::MYSQL), m_options(std::move(opt)) {}

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

    [[nodiscard]] QString getConnectionString() const {
        return std::visit(
            [](const auto& opt) {
                if constexpr (std::is_same_v<std::decay_t<decltype(opt)>, SqliteOptions>) {
                    return opt.dbName;
                } else {
                    return opt.getConnectionString();
                }
            },
            m_options);
    }

    [[nodiscard]] bool isValid() const {
        return std::visit([](const auto& opt) { return opt.isValid(); }, m_options);
    }

    // Enable_if Getter
    template <typename T>
    const T& get() const requires AllowedVariant<T> {
        return std::get<T>(m_options);
    }

private:
    Driver m_driver;
    Variant m_options;
};

#endif /* DATABASEOPTIONS_H */
