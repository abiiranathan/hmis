#ifndef DATABASE_H
#define DATABASE_H

#include <QList>
#include <QString>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <utility>
#include <variant>

#include "HMISRow.hpp"
#include "databaseOptions.hpp"

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

class Database {
private:
    const QString dxSeparator = "____";

    // Instance of the database
    QSqlDatabase db;
    ConnOptions m_connOptions;

public:
    // Default constructor
    Database();

    // Connect to the database with the given options. If the connection fails, it will throw a runtime exception.
    void Connect(const ConnOptions& options);

    // Create the tables. If an error occurs, it will throw a runtime exception.
    void createSchema();

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
