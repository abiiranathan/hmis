#include "database.h"

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

Database::Database() {}

bool Database::Connect() {
  db = QSqlDatabase::addDatabase("QSQLITE");
  const QString DB_NAME =
      QDir::cleanPath(QDir::homePath() + QDir::separator() + "hmis.sqlite3");

  auto envDbName = qgetenv("HMIS_DB");
  if (!envDbName.trimmed().isEmpty()) {
    db.setDatabaseName(envDbName);
  } else {
    db.setDatabaseName(DB_NAME);
  }

  if (!db.open()) {
    return false;
  }
  return createSchema();
}

bool Database::isOpen() { return db.isOpen(); }

bool Database::createSchema() {
  const QString schema =
      "CREATE TABLE IF NOT EXISTS hmis ("
      "id integer NOT NULL PRIMARY KEY AUTOINCREMENT, "
      "age_category TEXT NOT NULL,"
      "month int NOT NULL, "
      "year int NOT NULL, "
      "sex TEXT CHECK(sex IN ('Male', 'Female')) NOT NULL, "
      "new_attendance TEXT CHECK(new_attendance IN ('YES', 'NO')) NOT NULL "
      "DEFAULT 'YES', diagnosis TEXT DEFAULT '', ip_number varchar(100) "
      "DEFAULT '') ";
  // IP number column added for tracking patients
  QSqlQuery query(schema);
  return query.exec();
}

QString Database::getLastError() { return db.lastError().text(); }

HMISData Database::fetchHMISData(int year, int month) {
  QSqlQuery query;

  query.prepare("SELECT * FROM hmis WHERE year=:year AND month=:month");
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
      row.diagnoses = query.value(6).toString().split(dxSeparator);
      row.ipNumber = query.value(7).toString();
      rows << row;
    }
  }
  return rows;
}

bool Database::saveNewRow(const NewHMISData &data) {
  // Save this patient
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

  return query.exec();
}

bool Database::readFromBackup(QWidget *parent) {
  QString fileName =
      QFileDialog::getOpenFileName(parent, "Open database backup");

  if (fileName.isEmpty()) {
    qDebug() << "No file selected";
    return false;
  }

  // Create a database connection and open it
  QSqlDatabase backupDB =
      QSqlDatabase::addDatabase("QSQLITE", "backup_connection");
  backupDB.setDatabaseName(fileName);
  if (!backupDB.open()) {
    qDebug() << "Error: Failed to open backup database";
    return false;
  }

  // Copy data from the backup database to the current database
  QStringList tableNames = backupDB.tables(QSql::Tables);

  foreach (QString tableName, tableNames) {
    if (tableName != "hmis") {
      continue;
    }

    QSqlQuery selectQuery(backupDB);
    selectQuery.prepare(QString("SELECT * FROM %1").arg(tableName));

    if (!selectQuery.exec()) {
      qDebug() << "Error: Failed to select data from table" << tableName;
      continue;
    }

    QSqlQuery insertQuery(db);
    while (selectQuery.next()) {
      insertQuery.prepare(
          "INSERT INTO hmis (id, age_category, month, year, sex, "
          "new_attendance, "
          "diagnosis, ip_number) "
          "VALUES(:id, :age_category, :month, :year, :sex, :new_attendance, "
          ":diagnosis, :ip_number);");

      insertQuery.bindValue(":id", selectQuery.value(0).toInt());
      insertQuery.bindValue(":age_category", selectQuery.value(1).toString());
      insertQuery.bindValue(":month", selectQuery.value(2).toInt());
      insertQuery.bindValue(":year", selectQuery.value(3).toInt());
      insertQuery.bindValue(":sex", selectQuery.value(4).toString());
      insertQuery.bindValue(":new_attendance", selectQuery.value(5).toString());
      insertQuery.bindValue(":diagnosis", selectQuery.value(6).toString());
      insertQuery.bindValue(":ip_number", selectQuery.value(7).toString());

      if (!insertQuery.exec()) {
        qDebug() << "Error: Failed to insert data into table" << tableName;
        continue;
      }
    }
  }

  return true;
}

// returns the connections database name which may be empty.
QString Database::getDatabaseName() const { return db.databaseName(); }
