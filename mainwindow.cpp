#include "mainwindow.h"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>

#include "./ui_mainwindow.h"
#include "diagnosis.h"
#include "register.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  setWindowIcon(QIcon(":/favicon.ico"));
  setWindowTitle("HMIS 105");

  // Connect to the database and create all the schema
  // Database created in your home directory/hmis.sqlite3
  // To change the localtion set HMIS_DB environment variable.
  db.Connect();

  // Setup signals and slots
  connectSignals();

  // Build the UI
  initUI();

  // Set current date as last month
  // This will trigger data to be fetched
  QDate date = QDate().currentDate().addMonths(-1);
  ui->dateEdit->setDate(date);

  // set next ip number
  QString nextIP = db.nextIPNumber(date.year(), date.month());
  ui->IPN->setText(nextIP);
}

// Destructor
MainWindow::~MainWindow() { delete ui; }

void MainWindow::initUI() {
  // Populate all diagnoses
  Diagnosis dx;
  diagnoses = dx.getAllDiagnoses();
  ui->listWidgetAllDiagnoses->addItems(diagnoses);

  // Initialize stats maps
  SexMap sexMap;
  sexMap.insert(MALE, 0);
  sexMap.insert(FEMALE, 0);

  // Initialize ageCategory map
  AgeCategoryMap ageCategoryMap;
  ageCategoryMap.insert(ZERO_TO_TWENTY_EIGHT_DAYS, sexMap);
  ageCategoryMap.insert(TWENTY_NINE_DAYS_TO_FOUR_YEARS, sexMap);
  ageCategoryMap.insert(FIVE_TO_NINE_YEARS, sexMap);
  ageCategoryMap.insert(TEN_TO_NINETEEN_YEARS, sexMap);
  ageCategoryMap.insert(TWENTY_YEARS_AND_ABOVE, sexMap);

  // New attendance -> YES, Re-attendance -> NO
  attendanceStats.insert(YES, ageCategoryMap);
  attendanceStats.insert(NO, ageCategoryMap);

  // Initialize all diagnoses to zero counts
  for (QString &diagnosis : diagnoses) {
    diagnosisStats.insert(diagnosis, ageCategoryMap);
  }

  initializeTableWidget(ui->tableAttendances, 2);
  ui->tableAttendances->setStyleSheet(
      "QHeaderView::section {font-size:12px; } "
      "QHeaderView::section:nth-of-type(odd) { "
      "background-color: beige; color:purple; }");
  ui->tableAttendances->setMaximumHeight(85);

  initializeTableWidget(ui->tableDiagnoses, (int)diagnoses.size());
  ui->tableDiagnoses->setStyleSheet(
      "QHeaderView::section {font-size:12px;background-color: white; "
      "color:black; } ");

  QHeaderView *horizontal_header = ui->tableDiagnoses->horizontalHeader();
  horizontal_header->setStyleSheet(
      "QHeaderView::section {font-size:12px;background-color: white; "
      "color:black; } ");

  // create vetical header labels for attendances
  QStringList vHeaders = {"NEW ATTENDANCE", "RE-ATTENDANCE"};
  ui->tableAttendances->setVerticalHeaderLabels(vHeaders);

  // Set vertical headers for all diagnoses
  ui->tableDiagnoses->setVerticalHeaderLabels(diagnoses);

  // Set current age category
  ui->comboBoxCategory->setCurrentText(TWENTY_YEARS_AND_ABOVE);
}

void MainWindow::connectSignals() {
  connect(ui->listWidgetAllDiagnoses, &QListWidget::itemDoubleClicked, this,
          &MainWindow::addToSelectedDiagnoses);

  connect(ui->listWidgetSelected, &QListWidget::itemDoubleClicked, this,
          &MainWindow::removeFromSelectedDiagnoses);

  // When you type to filter diagnoses, update list widget
  connect(ui->lineEdit, &QLineEdit::textChanged, this,
          &MainWindow::diagnosisQueryChanged);

  connect(ui->dateEdit, &QDateEdit::dateChanged, this,
          &MainWindow::onDateChanged);

  // Connect form buttons
  connect(ui->btnSave, &QPushButton::clicked, this, &MainWindow::onSave);
  connect(ui->btnReset, &QPushButton::clicked, this, &MainWindow::onResetForm);

  // Connect menu actions
  connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onExit);
  connect(ui->actionExpand, &QAction::triggered, this,
          &MainWindow::onToggleSidebar);
  ui->actionToggle_Style->setVisible(false);
  connect(ui->actionView_Register, &QAction::triggered, this,
          &MainWindow::onViewRegister);

  connect(ui->actionBackup, &QAction::triggered, this,
          &MainWindow::onDatabaseBackup);
  connect(ui->actionRestore, &QAction::triggered, this,
          &MainWindow::onDatabaseRestore);

  // Hide empty diagnoses if checked
  connect(ui->checkHideEmpty, &QCheckBox::checkStateChanged, this,
          &MainWindow::toggleHideEmptyHiagnoses);
}

void MainWindow::filterDiagnoses(const QString &query) {
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

void MainWindow::toggleHideEmptyHiagnoses(Qt::CheckState state) {
  QAbstractItemModel *model = ui->tableDiagnoses->model();
  int columnCount = model->columnCount();
  int rowCount = model->rowCount();

  for (int row = 0; row < rowCount; ++row) {
    bool isRowEmpty = true;
    for (int col = 0; col < columnCount; ++col) {
      QModelIndex index =
          model->index(row, col);         // Get the index for the current cell
      QVariant data = model->data(index); // Get the data in the cell

      if (data.toInt() != 0) { // Check if the data is not zero
        isRowEmpty = false;
        break;
      }
    }

    if (state == Qt::Checked) {
      if (isRowEmpty) {
        ui->tableDiagnoses->setRowHidden(row, true);
      }
    } else {
      ui->tableDiagnoses->setRowHidden(row, false);
    }
  }
}

bool MainWindow::dbOpen() { return db.isOpen(); }

void MainWindow::onResetForm() {
  if (ui->noClearForm->isChecked()) {
    return;
  }

  // Reset the form
  if (!ui->checkKeepDiagnoses->isChecked()) {
    ui->listWidgetSelected->clear();
  }

  ui->comboBoxCategory->setCurrentText(TWENTY_YEARS_AND_ABOVE);
  ui->comboBoxNewAttendance->setCurrentText(YES);
  ui->comboBoxSex->setCurrentText(FEMALE);
}

void MainWindow::onSave() {
  QString ageCategory;
  QString newAttendance;
  QString sex;
  QString ipNumber;
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

  // Designated initializer syntax.
  // Fields must in declaration order in C++, unlike in C
  NewHMISData data = {.ageCategory = ageCategory,
                      .month = selectedDate.month(),
                      .year = selectedDate.year(),
                      .sex = sex,
                      .newAttendance = newAttendance,
                      .diagnoses = diagnosisList,
                      .ipNumber = ipNumber};

  if (db.saveNewRow(data)) {
    // Reset form
    onResetForm();

    // Refresh the attendances tableWidget
    populateAttendances(selectedDate.year(), selectedDate.month());

    // Refresh the diagnoses tableWidget
    populateDiagnoses(selectedDate.year(), selectedDate.month());

    // Show status message
    statusBar()->showMessage("Record inserted successfully", 5000);

    // Increment IP Number automatically
    if (!ipNumber.isEmpty()) {
      bool ok;
      int ipNumberInt = ipNumber.toInt(&ok);
      if (ok) {
        ipNumberInt++; // Increment
        // Format with leading zeros
        ipNumber = QString("%1").arg(ipNumberInt, 3, 10, QChar('0'));
        ui->IPN->setText(ipNumber); // Set the new IP Number
      }
    }
  } else {
    QMessageBox::critical(this, "Insert Error",
                          "Unable to insert row into the database");
  }
}

void MainWindow::initializeTableWidget(QTableWidget *w, int rowCount) {
  w->setColumnCount((int)diagnosisTableHeaders.size());
  w->setHorizontalHeaderLabels(diagnosisTableHeaders);

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

  w->setEditTriggers(QAbstractItemView::NoEditTriggers); // Disable cell editing
  w->setAlternatingRowColors(true);
  w->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  w->setRowCount(rowCount);
}

void MainWindow::populateAttendances(int year, int month) {
  HMISData rows = db.fetchHMISData(year, month);

  // Copy initial attendance stats map
  auto map = attendanceStats;

  // Update counts per category and sex
  for (HMISRow &row : rows) {
    map[row.newAttendance][row.ageCategory][row.sex]++;
  }

  // Attendances table has only two rows
  for (int row = 0; row < 2; row++) {
    QString newAttendance = row == 0 ? YES : NO;

    setAttendenceTableItem(row, 0,
                           map[newAttendance][ZERO_TO_TWENTY_EIGHT_DAYS][MALE]);
    setAttendenceTableItem(
        row, 1, map[newAttendance][ZERO_TO_TWENTY_EIGHT_DAYS][FEMALE]);

    setAttendenceTableItem(
        row, 2, map[newAttendance][TWENTY_NINE_DAYS_TO_FOUR_YEARS][MALE]);
    setAttendenceTableItem(
        row, 3, map[newAttendance][TWENTY_NINE_DAYS_TO_FOUR_YEARS][FEMALE]);

    setAttendenceTableItem(row, 4,
                           map[newAttendance][FIVE_TO_NINE_YEARS][MALE]);
    setAttendenceTableItem(row, 5,
                           map[newAttendance][FIVE_TO_NINE_YEARS][FEMALE]);

    setAttendenceTableItem(row, 6,
                           map[newAttendance][TEN_TO_NINETEEN_YEARS][MALE]);
    setAttendenceTableItem(row, 7,
                           map[newAttendance][TEN_TO_NINETEEN_YEARS][FEMALE]);

    setAttendenceTableItem(row, 8,
                           map[newAttendance][TWENTY_YEARS_AND_ABOVE][MALE]);
    setAttendenceTableItem(row, 9,
                           map[newAttendance][TWENTY_YEARS_AND_ABOVE][FEMALE]);
  }
}

void MainWindow::setAttendenceTableItem(int row, int column, int number) {
  ui->tableAttendances->setItem(row, column,
                                new QTableWidgetItem(QString::number(number)));
}

void MainWindow::setDiagnosisTableItem(int row, int column, int number) {
  ui->tableDiagnoses->setItem(row, column,
                              new QTableWidgetItem(QString::number(number)));
}

void MainWindow::populateDiagnoses(int year, int month) {
  HMISData rows = db.fetchHMISData(year, month);

  // Copy initial attendance stats map
  auto map = diagnosisStats;

  for (HMISRow &row : rows) {
    for (QString &dx : row.diagnoses) {
      // This will insert missing diagnoses as per std::unordered_map.
      map[dx][row.ageCategory][row.sex]++;
    }
  }

  // Add stanadard diagnoses
  for (int row = 0; row < diagnoses.size(); row++) {
    QString dx = diagnoses[row];
    if (!map.contains(dx)) {
      continue;
    }

    setDiagnosisTableItem(row, 0, map[dx][ZERO_TO_TWENTY_EIGHT_DAYS][MALE]);
    setDiagnosisTableItem(row, 1, map[dx][ZERO_TO_TWENTY_EIGHT_DAYS][FEMALE]);

    setDiagnosisTableItem(row, 2,
                          map[dx][TWENTY_NINE_DAYS_TO_FOUR_YEARS][MALE]);
    setDiagnosisTableItem(row, 3,
                          map[dx][TWENTY_NINE_DAYS_TO_FOUR_YEARS][FEMALE]);

    setDiagnosisTableItem(row, 4, map[dx][FIVE_TO_NINE_YEARS][MALE]);
    setDiagnosisTableItem(row, 5, map[dx][FIVE_TO_NINE_YEARS][FEMALE]);

    setDiagnosisTableItem(row, 6, map[dx][TEN_TO_NINETEEN_YEARS][MALE]);
    setDiagnosisTableItem(row, 7, map[dx][TEN_TO_NINETEEN_YEARS][FEMALE]);

    setDiagnosisTableItem(row, 8, map[dx][TWENTY_YEARS_AND_ABOVE][MALE]);
    setDiagnosisTableItem(row, 9, map[dx][TWENTY_YEARS_AND_ABOVE][FEMALE]);
  }
}

void MainWindow::addToSelectedDiagnoses(QListWidgetItem *item) {
  QStringList selectedItems;
  for (int i = 0; i < ui->listWidgetSelected->count(); ++i) {
    selectedItems << ui->listWidgetSelected->item(i)->text();
  }

  int row = (int)selectedItems.size();
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

void MainWindow::onDateChanged(const QDate &date) {
  currentYear = date.year();
  currentMonth = date.month();
  populateAttendances(currentYear, currentMonth);
  populateDiagnoses(currentYear, currentMonth);
}

void MainWindow::onToggleSidebar(bool toggled) {
  ui->sidebar->setVisible(toggled);
  if (toggled) {
    ui->actionExpand->setIcon(QIcon(":/icons/toggle-on.png"));
  } else {
    ui->actionExpand->setIcon(QIcon(":/icons/toggle-off.png"));
  }
}

void MainWindow::onExit() { qApp->quit(); }

void MainWindow::onDatabaseBackup() {
  const QString dirName =
      QFileDialog::getExistingDirectory(this, "Backup directory", QString());
  if (dirName.isEmpty()) {
    return;
  }

  QString databasePath = db.getDatabaseName();
  if (QFile::exists(databasePath)) {
    QFileInfo fileInfo(databasePath);

    QString backupName =
        QDir::cleanPath(dirName + QDir::separator() + fileInfo.baseName() +
                        "." + fileInfo.suffix());

    QFile srcFile(databasePath);

    if (!srcFile.copy(backupName)) {
      srcFile.close();
      QMessageBox::warning(this, "IO Error",
                           "Error copying backup to disk: " +
                               srcFile.errorString());
      return;
    }

    srcFile.close();
    QMessageBox::information(this, "Success", "database backup created!");
  } else {
    QMessageBox::warning(this, "Not Found", "database file not found!");
  }
}

void MainWindow::onDatabaseRestore() {
  if (db.readFromBackup(this)) {
    QMessageBox::information(this, "Restore", "Database restore complete");
    QDate date = ui->dateEdit->date();
    int year = date.year();
    int month = date.month();
    populateAttendances(year, month);
    populateDiagnoses(year, month);
    return;
  }

  QMessageBox::warning(this, "Restore Failed", "Database restore failed");
}

void MainWindow::onViewRegister() {
  QDate date = ui->dateEdit->date();

  int year = date.year();
  int month = date.month();
  auto *registerDialog = new Register(&db, this);

  registerDialog->setData(db.fetchHMISData(year, month));
  registerDialog->showMaximized();
}

void MainWindow::diagnosisQueryChanged(const QString &query) {
  filterDiagnoses(query);
}
