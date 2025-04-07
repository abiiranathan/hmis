#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QListWidgetItem>
#include <QMainWindow>
#include <QTableWidget>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "database.h"

typedef QString Sex;
const Sex MALE   = QString("Male");    // NOLINT
const Sex FEMALE = QString("Female");  // NOLINT

// Age categories
const QString ZERO_TO_TWENTY_EIGHT_DAYS      = "0 - 28 days";
const QString TWENTY_NINE_DAYS_TO_FOUR_YEARS = "29 days - 4 years";
const QString FIVE_TO_NINE_YEARS             = "5 - 9 years";
const QString TEN_TO_NINETEEN_YEARS          = "10 - 19 years";
const QString TWENTY_YEARS_AND_ABOVE         = "20 years and above";

// Attendance statuses
const QString YES = "YES";
const QString NO  = "NO";

const QStringList diagnosisTableHeaders = {
    "0 - 28d(M)",
    "0 - 28d(F)",
    "29d - 4yrs(M)",
    "29d - 4yrs(F)",
    "5 - 9yrs(M)",
    "5 - 9yrs(F)",
    "10 - 19yrs(M)",
    "10 - 19yrs(F)",
    ">=20yrs(M)",
    ">=20yrs(F)",
};

// QMap storing counts for different sexes
typedef QMap<Sex, int> SexMap;

typedef QMap<QString, SexMap> AgeCategoryMap;

// map[attendance_status][category][sex]
typedef QMap<QString, QMap<QString, SexMap>> AttendanceMap;

// map[diagnosis][category][sex]
typedef QMap<QString, QMap<QString, SexMap>> DiagnosisMap;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    // public members
    void filterDiagnoses(const QString& query);
    bool dbOpen();

private slots:
    void onResetForm();
    void onSave();
    void diagnosisQueryChanged(const QString& query);
    void addToSelectedDiagnoses(QListWidgetItem* item);
    void removeFromSelectedDiagnoses(QListWidgetItem* item);
    void onDateChanged(const QDate& date);
    void onToggleSidebar(bool toggled);
    void onViewRegister();
    void onExit();
    void onDatabaseBackup();
    void onDatabaseRestore();

private:
    Ui::MainWindow* ui;
    Database db;
    QList<QString> diagnoses;
    QList<QString> matchingDiagnoses;
    QString diagnosisQuery;

    // Holds data for attandances
    AttendanceMap attendanceStats;

    // Holds data for diagnoses per month
    DiagnosisMap diagnosisStats;

    void initializeTableWidget(QTableWidget* w, int rowCount);
    void populateAttendances(int year, int month);
    void populateDiagnoses(int year, int month);
    void connectSignals();

    // Initialialize the UI
    void initUI();
};

#endif  // MAINWINDOW_H
