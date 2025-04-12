#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QListWidgetItem>
#include <QMainWindow>
#include <QTableWidget>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "database.hpp"
#include "register.hpp"

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

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

    Ui::MainWindow* ui;
    Database db;
    QList<Diagnosis> diagnoses;
    QStringList diagnosisNames;

    QList<QString> matchingDiagnoses;
    QString diagnosisQuery;

    int currentYear;
    int currentMonth;

    // Holds data for attandances
    StatsMap attendanceStats;

    // Holds data for diagnoses per month
    StatsMap diagnosisStats;

    void initializeTableWidget(QTableWidget* w, int rowCount);
    void populateAttendances(int year, int month);
    void populateDiagnoses(int year, int month);
    void connectSignals();

    // Initialialize the UI
    void initUI();
    void setDiagnosisTableItem(int row, int column, int number);
    void setAttendenceTableItem(int row, int column, int number);

public:
    MainWindow(Database& conn, QWidget* parent = nullptr);
    ~MainWindow() override;

    // public members
    void filterDiagnoses(const QString& query);

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
    void toggleHideEmptyHiagnoses(Qt::CheckState state);
    void filterVisibleDiagnoses(const QString& query);
    void onViewDiagnoses();
    void onAddDiagnosis();
};

#endif  // MAINWINDOW_H
