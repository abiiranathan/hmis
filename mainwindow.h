#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QListWidgetItem>
#include <QMainWindow>
#include <QTableWidget>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "HMISRow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  bool connectToDatabase();
  bool createTableSchema();

  // public members
  void filterDiagnoses(QString query);
  bool dbOpen();

 private slots:
  void on_btnReset_clicked();
  void on_btnSave_clicked();

  void diagnosisQueryChanged(QString query);
  void addToSelectedDiagnoses(QListWidgetItem *item);
  void removeFromSelectedDiagnoses(QListWidgetItem *item);
  void on_dateEdit_userDateChanged(const QDate &date);
  void on_actionExpand_toggled(bool arg1);
  void on_actionView_Register_triggered();
  void on_actionToggle_Style_toggled(bool arg1);
  void on_actionExit_triggered();

 private:
  Ui::MainWindow *ui;
  QList<QString> diagnoses;
  QList<QString> matchingDiagnoses;
  QString diagnosisQuery;
  const QString dxSeparator = "____";

  QSqlDatabase db;

  void initializeTableWidget(QTableWidget *w, int rowCount);
  void populateAttendances(int year, int month);
  void populateDiagnoses(int year, int month);

  QList<HMISRow> fetchHMISData(int year, int month);

  // Holds data for attandances
  // map[attendance_status][category][sex]
  QMap<QString, QMap<QString, QMap<QString, int>>> attendanceStats;

  // Holds data for diagnoses per month
  // map[diagnosis][category][sex]
  QMap<QString, QMap<QString, QMap<QString, int>>> diagnosisStats;
};

#endif  // MAINWINDOW_H
