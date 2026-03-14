#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLabel>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QTableWidget>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <optional>

#include "database.hpp"
#include "register.hpp"

const QStringList diagnosisTableHeaders = {
    "0-28d(M)", "0-28d(F)",
    "29d-4y(M)", "29d-4y(F)",
    "5-9y(M)", "5-9y(F)",
    "10-19y(M)", "10-19y(F)",
    "≥20y(M)", "≥20y(F)",
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

    Ui::MainWindow* ui;
    Database& db;             // reference — no copy
    User m_currentUser;       // logged-in user

    QList<Diagnosis> diagnoses;
    QStringList diagnosisNames;
    QList<QString> matchingDiagnoses;

    int currentYear;
    int currentMonth;

    void initializeTableWidget(QTableWidget* w, int rowCount);
    void populateAttendances(int year, int month);
    void populateDiagnoses(int year, int month);
    void connectSignals();
    void initUI();
    void updateDashboard(int year, int month);
    void setDiagnosisTableItem(int row, int column, int number);
    void setAttendanceTableItem(int row, int column, int number);

    // Input validation — returns list of error strings (empty = OK)
    QStringList validateForm() const;

public:
    MainWindow(Database& conn, const User& user, QWidget* parent = nullptr);
    ~MainWindow() override;

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
    void toggleHideEmptyDiagnoses(Qt::CheckState state);
    void filterVisibleDiagnoses(const QString& query);
    void onViewDiagnoses();
    void onAddDiagnosis();
    void onExportCSV();
    void onBackupDatabase();
    void onViewAuditLog();
    void onManageUsers();
    void onChangePassword();
};

#endif  // MAINWINDOW_H
