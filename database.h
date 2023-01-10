#ifndef DATABASE_H
#define DATABASE_H
#include <QList>
#include <QString>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "HMISRow.h"

// HMISData alias
typedef QList<HMISRow> HMISData;

// Data for creating a new record
typedef struct NewHMISData {
  QString ageCategory;
  int month;
  int year;
  QString sex;
  QString newAttendance;
  QList<QString> diagnoses;
  QString ipNumber;
} NewHMISData;

// Encapsulates all the database related logic like connection to db,
// creating tables, executing queries, fetching data.
class Database {
 private:
  const QString dxSeparator = "____";

  // Instance of the database
  QSqlDatabase db;

  // Create the tables. If an error occurs, returns false.
  bool createSchema();

 public:
  // Default constructor
  Database();

  // Connect to the database. If the connection fails, returns false.
  // Get the error by calling getLastError() method.
  bool Connect();

  // returns true if the database connection is open
  bool isOpen();

  // Returns the last database connection error. If there is no error, returns
  // "".
  QString getLastError();

  // Fetch data for a given month and year.
  HMISData fetchHMISData(int year, int month);

  // Save new HMIS row
  bool saveNewRow(const NewHMISData &data);

  // Read data from backup and insert it to db
  bool readFromBackup(QWidget *parent);

  // returns the path to db
  QString getDatabaseName() const;
};

#endif  // DATABASE_H
