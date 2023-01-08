#include "mainwindow.h"

#include <HMISRow.h>

#include <QApplication>
#include <QMessageBox>

#include "./ui_mainwindow.h"
#include "diagnosis.h"
#include "register.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  setWindowIcon(QIcon(":/favicon.ico"));
  setWindowTitle("HMIS 105");

  connectToDatabase();

  // Populate all diagnoses
  Diagnosis dx;
  diagnoses = dx.getAllDiagnoses();
  ui->listWidgetAllDiagnoses->addItems(diagnoses);

  connect(ui->listWidgetAllDiagnoses, &QListWidget::itemDoubleClicked, this,
          &MainWindow::addToSelectedDiagnoses);

  connect(ui->listWidgetSelected, &QListWidget::itemDoubleClicked, this,
          &MainWindow::removeFromSelectedDiagnoses);

  // Initialize stats maps
  QMap<QString, int> sexMap;
  sexMap.insert("Male", 0);
  sexMap.insert("Female", 0);

  QMap<QString, QMap<QString, int>> ageCategoryMap;
  ageCategoryMap.insert("0 - 28 days", sexMap);
  ageCategoryMap.insert("29 days - 4 years", sexMap);
  ageCategoryMap.insert("5 - 9 years", sexMap);
  ageCategoryMap.insert("10 - 19 years", sexMap);
  ageCategoryMap.insert("20 years and above", sexMap);

  // New attendance -> YES, Re-attendance -> NO
  attendanceStats.insert("YES", ageCategoryMap);
  attendanceStats.insert("NO", ageCategoryMap);

  // Initialize all diagnoses to zero counts
  for (QString &dx : diagnoses) {
    diagnosisStats.insert(dx, ageCategoryMap);
  }

  QString stylesheet =
      "QHeaderView::section {font-size:12px; } "
      "QHeaderView::section:nth-of-type(odd) { "
      "background-color: beige; color:purple; }";

  initializeTableWidget(ui->tableAttendances, 2);
  ui->tableAttendances->setStyleSheet(stylesheet);
  ui->tableAttendances->setMaximumHeight(85);

  initializeTableWidget(ui->tableDiagnoses, diagnoses.size());
  ui->tableDiagnoses->setStyleSheet(
      "QHeaderView::section {font-size:12px;background-color: white; "
      "color:black; } ");

  auto horizontal_header = ui->tableDiagnoses->horizontalHeader();
  horizontal_header->setStyleSheet(stylesheet);

  // create vetical header labels for attendances
  QStringList vHeaders = {"NEW ATTENDANCE", "RE-ATTENDANCE"};
  ui->tableAttendances->setVerticalHeaderLabels(vHeaders);

  // Set vertical headers for all diagnoses
  ui->tableDiagnoses->setVerticalHeaderLabels(diagnoses);

  // When you type to filter diagnoses, update list widget
  connect(ui->lineEdit, &QLineEdit::textChanged, this,
          &MainWindow::diagnosisQueryChanged);

  // Set current date as last month
  ui->dateEdit->setDate(QDate().currentDate().addMonths(-1));

  // Set current age category
  ui->comboBoxCategory->setCurrentText("20 years and above");
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::filterDiagnoses(QString query) {
  if (query.trimmed().isEmpty()) {
    // Show all diagnoses
    ui->listWidgetAllDiagnoses->clear();
    ui->listWidgetAllDiagnoses->addItems(diagnoses);
    return;
  }

  matchingDiagnoses = {};
  foreach (QString d, diagnoses) {
    if (d.contains(query, Qt::CaseInsensitive)) {
      matchingDiagnoses.append(d);
    }
  }

  ui->listWidgetAllDiagnoses->clear();
  ui->listWidgetAllDiagnoses->addItems(matchingDiagnoses);
}

bool MainWindow::dbOpen() { return db.isOpen(); }

void MainWindow::on_btnReset_clicked() {
  // Reset the form
  ui->listWidgetSelected->clear();
  ui->comboBoxCategory->setCurrentText("20 years and above");
  ui->comboBoxNewAttendance->setCurrentText("YES");
  ui->comboBoxSex->setCurrentText("Female");
}

void MainWindow::on_btnSave_clicked() {
  // Get data from ui form
  QString ageCategory, newAttendance, sex, ipNumber;
  QDate selectedDate;
  QStringList diagnosisList;

  ageCategory = ui->comboBoxCategory->currentText();
  newAttendance = ui->comboBoxNewAttendance->currentText();
  sex = ui->comboBoxSex->currentText();
  selectedDate = ui->dateEdit->date();
  ipNumber = ui->IPN->text();

  for (int i = 0; i < ui->listWidgetSelected->count(); ++i) {
    diagnosisList << ui->listWidgetSelected->item(i)->text();
  }

  // Save this patient
  QSqlQuery query;

  query.prepare(
      "INSERT INTO hmis (age_category, month, year, sex, new_attendance, "
      "diagnosis, ip_number) "
      "VALUES(:age_category, :month, :year, :sex, :new_attendance, "
      ":diagnosis, :ip_number);");

  query.bindValue(":age_category", ageCategory);
  query.bindValue(":month", selectedDate.month());
  query.bindValue(":year", selectedDate.year());
  query.bindValue(":sex", sex);
  query.bindValue(":new_attendance", newAttendance);
  query.bindValue(":diagnosis", diagnosisList.join(dxSeparator));
  query.bindValue(":ip_number", ipNumber);

  if (query.exec()) {
    on_btnReset_clicked();

    // Refresh the attendances tableWidget
    populateAttendances(selectedDate.year(), selectedDate.month());
    // Refresh the diagnoses tableWidget
    populateDiagnoses(selectedDate.year(), selectedDate.month());

    statusBar()->showMessage("Record inserted successfully", 3000);
  } else {
    QMessageBox::critical(this, "Error", query.lastError().text());
  }
}

void MainWindow::diagnosisQueryChanged(QString query) {
  filterDiagnoses(query);
}

void MainWindow::addToSelectedDiagnoses(QListWidgetItem *item) {
  QStringList selectedItems;
  for (int i = 0; i < ui->listWidgetSelected->count(); ++i) {
    selectedItems << ui->listWidgetSelected->item(i)->text();
  }

  int row = selectedItems.size();
  if (selectedItems.contains(item->text())) {
    QMessageBox::information(this, "Duplicate diagnosis",
                             item->text() + " already added!");
    return;
  }
  ui->listWidgetSelected->insertItem(row, item->text());
}

void MainWindow::removeFromSelectedDiagnoses(QListWidgetItem *item) {
  delete ui->listWidgetSelected->takeItem(ui->listWidgetSelected->row(item));
}

void MainWindow::initializeTableWidget(QTableWidget *w, int rowCount) {
  // Set all tableView Headers
  QStringList headers = {
      "0 - 28d(M)",  "0 - 28d(F)",  "29d - 4yrs(M)", "29d - 4yrs(F)",
      "5 - 9yrs(M)", "5 - 9yrs(F)", "10 - 19yrs(M)", "10 - 19yrs(F)",
      ">=20yrs(M)",  ">=20yrs(F)",
  };

  // Show grid to show headers
  w->setColumnCount(headers.size());
  w->setHorizontalHeaderLabels(headers);

  QHeaderView *horizontalHeader = w->horizontalHeader();
  horizontalHeader->setSectionResizeMode(0, QHeaderView::Stretch);
  horizontalHeader->setSectionResizeMode(1, QHeaderView::Stretch);
  horizontalHeader->setSectionResizeMode(2, QHeaderView::Stretch);
  horizontalHeader->setSectionResizeMode(3, QHeaderView::Stretch);
  horizontalHeader->setSectionResizeMode(4, QHeaderView::Stretch);
  horizontalHeader->setSectionResizeMode(5, QHeaderView::Stretch);
  horizontalHeader->setSectionResizeMode(6, QHeaderView::Stretch);
  horizontalHeader->setSectionResizeMode(7, QHeaderView::Stretch);
  horizontalHeader->setSectionResizeMode(8, QHeaderView::Stretch);
  horizontalHeader->setSectionResizeMode(9, QHeaderView::Stretch);

  horizontalHeader->setStyleSheet(
      "font-family: Arial; font-size: 12px; background-color: lightgray;");

  w->setEditTriggers(
      QAbstractItemView::NoEditTriggers);  // Disable cell editing
  w->setAlternatingRowColors(true);
  w->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  w->setRowCount(rowCount);
}

void MainWindow::populateAttendances(int year, int month) {
  QList<HMISRow> rows = fetchHMISData(year, month);

  // Copy initial attendance stats map
  QMap<QString, QMap<QString, QMap<QString, int>>> map = attendanceStats;

  // Update counts per category and sex
  for (HMISRow &row : rows) {
    map[row.newAttendance][row.ageCategory][row.sex]++;
  }

  int row;
  for (row = 0; row < 2; row++) {
    QString newAttendance = row == 0 ? "YES" : "NO";

    // Neonates
    ui->tableAttendances->setItem(
        row, 0,
        new QTableWidgetItem(
            QString::number(map[newAttendance]["0 - 28 days"]["Male"])));
    ui->tableAttendances->setItem(
        row, 1,
        new QTableWidgetItem(
            QString::number(map[newAttendance]["0 - 28 days"]["Female"])));

    // Infants and toddlers
    ui->tableAttendances->setItem(
        row, 2,
        new QTableWidgetItem(
            QString::number(map[newAttendance]["29 days - 4 years"]["Male"])));
    ui->tableAttendances->setItem(
        row, 3,
        new QTableWidgetItem(QString::number(
            map[newAttendance]["29 days - 4 years"]["Female"])));

    // school-going children
    ui->tableAttendances->setItem(
        row, 4,
        new QTableWidgetItem(
            QString::number(map[newAttendance]["5 - 9 years"]["Male"])));
    ui->tableAttendances->setItem(
        row, 5,
        new QTableWidgetItem(
            QString::number(map[newAttendance]["5 - 9 years"]["Female"])));

    // Adolescents
    ui->tableAttendances->setItem(
        row, 6,
        new QTableWidgetItem(
            QString::number(map[newAttendance]["10 - 19 years"]["Male"])));
    ui->tableAttendances->setItem(
        row, 7,
        new QTableWidgetItem(
            QString::number(map[newAttendance]["10 - 19 years"]["Female"])));

    // Adults above 20
    ui->tableAttendances->setItem(
        row, 8,
        new QTableWidgetItem(
            QString::number(map[newAttendance]["20 years and above"]["Male"])));
    ui->tableAttendances->setItem(
        row, 9,
        new QTableWidgetItem(QString::number(
            map[newAttendance]["20 years and above"]["Female"])));
  }
}

void MainWindow::populateDiagnoses(int year, int month) {
  QList<HMISRow> rows = fetchHMISData(year, month);

  // Copy initial attendance stats map
  QMap<QString, QMap<QString, QMap<QString, int>>> map = diagnosisStats;

  for (HMISRow &row : rows) {
    for (QString &dx : row.diagnoses) {
      map[dx][row.ageCategory][row.sex]++;
    }
  }

  int row = 0;
  for (row = 0; row < diagnoses.size(); row++) {
    QString dx = diagnoses[row];
    if (!map.contains(dx)) {
      continue;
    }

    int column = 0;
    ui->tableDiagnoses->setItem(
        row, column,
        new QTableWidgetItem(QString::number(map[dx]["0 - 28 days"]["Male"])));
    ui->tableDiagnoses->setItem(row, column + 1,
                                new QTableWidgetItem(QString::number(
                                    map[dx]["0 - 28 days"]["Female"])));

    // Infants and toddlers
    ui->tableDiagnoses->setItem(row, column + 2,
                                new QTableWidgetItem(QString::number(
                                    map[dx]["29 days - 4 years"]["Male"])));
    ui->tableDiagnoses->setItem(row, column + 3,
                                new QTableWidgetItem(QString::number(
                                    map[dx]["29 days - 4 years"]["Female"])));

    // school-going children
    ui->tableDiagnoses->setItem(
        row, column + 4,
        new QTableWidgetItem(QString::number(map[dx]["5 - 9 years"]["Male"])));
    ui->tableDiagnoses->setItem(row, column + 5,
                                new QTableWidgetItem(QString::number(
                                    map[dx]["5 - 9 years"]["Female"])));

    // Adolescents
    ui->tableDiagnoses->setItem(row, column + 6,
                                new QTableWidgetItem(QString::number(
                                    map[dx]["10 - 19 years"]["Male"])));
    ui->tableDiagnoses->setItem(row, column + 7,
                                new QTableWidgetItem(QString::number(
                                    map[dx]["10 - 19 years"]["Female"])));

    // Adults above 20
    ui->tableDiagnoses->setItem(row, column + 8,
                                new QTableWidgetItem(QString::number(
                                    map[dx]["20 years and above"]["Male"])));
    ui->tableDiagnoses->setItem(row, column + 9,
                                new QTableWidgetItem(QString::number(
                                    map[dx]["20 years and above"]["Female"])));
  }
}

QList<HMISRow> MainWindow::fetchHMISData(int year, int month) {
  QSqlQuery query;

  query.prepare("SELECT * FROM hmis WHERE year=:year AND month=:month");
  query.bindValue(":year", year);
  query.bindValue(":month", month);

  QList<HMISRow> rows;

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

bool MainWindow::connectToDatabase() {
  const QString DBNAME = "db.sqlite3";
  db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName(DBNAME);

  if (!db.open()) {
    QMessageBox::critical(this, "db error", db.lastError().text());
    return false;
  }

  return createTableSchema();
}

bool MainWindow::createTableSchema() {
  QString schema =
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

void MainWindow::on_dateEdit_userDateChanged(const QDate &date) {
  int year = date.year();
  int month = date.month();
  populateAttendances(year, month);
  populateDiagnoses(year, month);
}

void MainWindow::on_actionExpand_toggled(bool arg1) {
  ui->sidebar->setVisible(arg1);
}

void MainWindow::on_actionExit_triggered() { qApp->quit(); }

void MainWindow::on_actionView_Register_triggered() {
  // Show the register window
  QDate date = ui->dateEdit->date();

  int year = date.year();
  int month = date.month();

  Register *registerDialog = new Register(this);
  registerDialog->setData(fetchHMISData(year, month));
  registerDialog->showMaximized();
}

void MainWindow::on_actionToggle_Style_toggled(bool arg1) {
  if (arg1) {
    qApp->setStyle("Fusion");
  } else {
    qApp->setStyle("Windows");
  }
}
