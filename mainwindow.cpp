
#include <QApplication>
#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>

#include "./ui_mainwindow.h"

#include "mainwindow.hpp"
#include "register.hpp"

MainWindow::MainWindow(Database& conn, QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow), db(conn) {
    ui->setupUi(this);
    setWindowIcon(QIcon(":/favicon.ico"));
    setWindowTitle("HMIS 105");
    menuBar()->hide();

    // Setup signals and slots
    connectSignals();

    // Build the UI
    initUI();

    // Set current date as last month
    // This will trigger data to be fetched
    QDate date = QDate().currentDate().addMonths(-1);
    ui->dateEdit->setDate(date);
    ui->dateEdit->setMaximumDate(QDate::currentDate());

    // set next ip number
    QString nextIP = db.nextIPNumber(date.year(), date.month());
    ui->IPN->setText(nextIP);
}

// Destructor
MainWindow::~MainWindow() {
    delete ui;
}

// Read default diagnosis names from resource file
static QStringList readDefaultDiagnoses(QWidget* parent) {
    QFile file(":/diagnoses.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(parent, "Error", "Unable to open diagnoses file");
        return {};
    }

    QStringList diagnoses;
    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        QString str     = QString::fromUtf8(line).trimmed();
        if (!str.isEmpty()) {
            diagnoses << str;
        }
    }
    return diagnoses;
}

void MainWindow::initUI() {
    // Populate all diagnoses
    auto storedDiagnoses = db.getAllDiagnoses();
    if (!storedDiagnoses) {
        QMessageBox::critical(this, "Error", "Unable to fetch diagnoses from the database");
        qApp->exit(1);
    }

    // Store diagnoses in a member variable
    diagnoses = storedDiagnoses.value();
    if (diagnoses.isEmpty()) {
        // Insert default diagnoses
        if (!db.insertDiagnoses(readDefaultDiagnoses(this))) {
            QMessageBox::critical(this, "Error", "Unable to insert default diagnoses");
            qApp->exit(1);
        }

        // Re-fetch diagnoses from the database
        storedDiagnoses = db.getAllDiagnoses();
        if (!storedDiagnoses) {
            QMessageBox::critical(this, "Error", "Unable to fetch diagnoses from the database");
            qApp->exit(1);
        }

        // Store diagnoses in a member variable
        diagnoses = storedDiagnoses.value();
        if (diagnoses.isEmpty()) {
            QMessageBox::critical(this, "Error", "No diagnoses found in the database");
            qApp->exit(1);
        }
    }

    // Populate the list widget with all diagnoses
    ui->listWidgetAllDiagnoses->clear();

    diagnosisNames.clear();

    // Populate the list widget with all diagnoses
    for (const Diagnosis& d : diagnoses) {
        diagnosisNames << d.name;
    }

    ui->listWidgetAllDiagnoses->addItems(diagnosisNames);

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
    for (QString& diagnosis : diagnosisNames) {
        diagnosisStats.insert(diagnosis, ageCategoryMap);
    }

    initializeTableWidget(ui->tableAttendances, 2);
    ui->tableAttendances->setStyleSheet(
        "QHeaderView::section {font-size:12px; } "
        "QHeaderView::section:nth-of-type(odd) { "
        "background-color: beige; color:purple; }");
    ui->tableAttendances->setMaximumHeight(85);

    initializeTableWidget(ui->tableDiagnoses, (int)diagnosisNames.size());
    ui->tableDiagnoses->setStyleSheet(
        "QHeaderView::section {font-size:14px;background-color: white; "
        "color:black; } ");

    QHeaderView* horizontal_header = ui->tableDiagnoses->horizontalHeader();
    horizontal_header->setStyleSheet("QHeaderView::section {font-size:11px; background-color: beige; color:purple;}");

    // create vetical header labels for attendances
    QStringList vHeaders = {"NEW ATTENDANCE", "RE-ATTENDANCE"};
    ui->tableAttendances->setVerticalHeaderLabels(vHeaders);

    // Set vertical headers for all diagnoses
    ui->tableDiagnoses->setVerticalHeaderLabels(diagnosisNames);

    // Set current age category
    ui->comboBoxCategory->setCurrentText(TWENTY_YEARS_AND_ABOVE);
}

void MainWindow::connectSignals() {
    connect(ui->listWidgetAllDiagnoses, &QListWidget::itemDoubleClicked, this, &MainWindow::addToSelectedDiagnoses);

    connect(ui->listWidgetSelected, &QListWidget::itemDoubleClicked, this, &MainWindow::removeFromSelectedDiagnoses);

    // When you type to filter diagnoses, update list widget
    connect(ui->lineEdit, &QLineEdit::textChanged, this, &MainWindow::diagnosisQueryChanged);

    connect(ui->dateEdit, &QDateEdit::dateChanged, this, &MainWindow::onDateChanged);

    // Connect form buttons
    connect(ui->btnSave, &QPushButton::clicked, this, &MainWindow::onSave);
    connect(ui->btnReset, &QPushButton::clicked, this, &MainWindow::onResetForm);

    // Connect menu actions
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onExit);
    connect(ui->actionExpand, &QAction::triggered, this, &MainWindow::onToggleSidebar);

    connect(ui->actionView_Register, &QAction::triggered, this, &MainWindow::onViewRegister);

    // Hide empty diagnoses if checked
    connect(ui->checkHideEmpty, &QCheckBox::checkStateChanged, this, &MainWindow::toggleHideEmptyHiagnoses);

    // Search diagnosis table by name
    connect(ui->txtFilter, &QLineEdit::textChanged, this, &MainWindow::filterVisibleDiagnoses);

    // actionViewall diagnoses
    connect(ui->actionView_All_Diagnoses, &QAction::triggered, this, &MainWindow::onViewDiagnoses);
    connect(ui->actionRegister_New_Diagnosis, &QAction::triggered, this, &MainWindow::onAddDiagnosis);
}

void MainWindow::filterDiagnoses(const QString& query) {
    if (query.trimmed().isEmpty()) {
        // Show all diagnoses
        ui->listWidgetAllDiagnoses->clear();
        ui->listWidgetAllDiagnoses->addItems(diagnosisNames);
        return;
    }

    matchingDiagnoses = {};
    foreach (QString d, diagnosisNames) {
        if (d.contains(query, Qt::CaseInsensitive)) {
            matchingDiagnoses.append(d);
        }
    }

    ui->listWidgetAllDiagnoses->clear();
    ui->listWidgetAllDiagnoses->addItems(matchingDiagnoses);
}

void MainWindow::toggleHideEmptyHiagnoses(Qt::CheckState state) {
    QAbstractItemModel* model = ui->tableDiagnoses->model();
    int columnCount           = model->columnCount();
    int rowCount              = model->rowCount();

    for (int row = 0; row < rowCount; ++row) {
        bool isRowEmpty = true;
        for (int col = 0; col < columnCount; ++col) {
            QModelIndex index = model->index(row, col);  // Get the index for the current cell
            QVariant data     = model->data(index);      // Get the data in the cell

            if (data.toInt() != 0) {  // Check if the data is not zero
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

    ageCategory   = ui->comboBoxCategory->currentText();
    newAttendance = ui->comboBoxNewAttendance->currentText();
    sex           = ui->comboBoxSex->currentText();
    selectedDate  = ui->dateEdit->date();
    ipNumber      = ui->IPN->text();

    for (int i = 0; i < ui->listWidgetSelected->count(); ++i) {
        diagnosisList << ui->listWidgetSelected->item(i)->text();
    }

    // Fields must in declaration order in C++, unlike in C
    NewHMISData data = {
        .ageCategory   = ageCategory,
        .sex           = sex,
        .newAttendance = newAttendance,
        .diagnoses     = diagnosisList,
        .ipNumber      = ipNumber,
        .month         = selectedDate.month(),
        .year          = selectedDate.year(),
    };

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
                ipNumberInt++;  // Increment
                // Format with leading zeros
                ipNumber = QString("%1").arg(ipNumberInt, 3, 10, QChar('0'));
                ui->IPN->setText(ipNumber);  // Set the new IP Number
            }
        }
    } else {
        QMessageBox::critical(this, "Insert Error", "Unable to insert row into the database");
    }
}

void MainWindow::initializeTableWidget(QTableWidget* w, int rowCount) {
    w->setColumnCount((int)diagnosisTableHeaders.size());
    w->setHorizontalHeaderLabels(diagnosisTableHeaders);

    QHeaderView* horizontalHeader = w->horizontalHeader();
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

    horizontalHeader->setStyleSheet("font-family: Arial; font-size: 12px; background-color: lightgray;");

    w->setEditTriggers(QAbstractItemView::NoEditTriggers);  // Disable cell editing
    w->setAlternatingRowColors(true);
    w->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    w->setRowCount(rowCount);
}

void MainWindow::populateAttendances(int year, int month) {
    HMISData rows = db.fetchHMISData(year, month);

    // Copy initial attendance stats map
    auto map = attendanceStats;

    // Update counts per category and sex
    for (HMISRow& row : rows) {
        map[row.newAttendance][row.ageCategory][row.sex]++;
    }

    // Attendances table has only two rows
    for (int row = 0; row < 2; row++) {
        QString newAttendance = row == 0 ? YES : NO;

        setAttendenceTableItem(row, 0, map[newAttendance][ZERO_TO_TWENTY_EIGHT_DAYS][MALE]);
        setAttendenceTableItem(row, 1, map[newAttendance][ZERO_TO_TWENTY_EIGHT_DAYS][FEMALE]);

        setAttendenceTableItem(row, 2, map[newAttendance][TWENTY_NINE_DAYS_TO_FOUR_YEARS][MALE]);
        setAttendenceTableItem(row, 3, map[newAttendance][TWENTY_NINE_DAYS_TO_FOUR_YEARS][FEMALE]);

        setAttendenceTableItem(row, 4, map[newAttendance][FIVE_TO_NINE_YEARS][MALE]);
        setAttendenceTableItem(row, 5, map[newAttendance][FIVE_TO_NINE_YEARS][FEMALE]);

        setAttendenceTableItem(row, 6, map[newAttendance][TEN_TO_NINETEEN_YEARS][MALE]);
        setAttendenceTableItem(row, 7, map[newAttendance][TEN_TO_NINETEEN_YEARS][FEMALE]);

        setAttendenceTableItem(row, 8, map[newAttendance][TWENTY_YEARS_AND_ABOVE][MALE]);
        setAttendenceTableItem(row, 9, map[newAttendance][TWENTY_YEARS_AND_ABOVE][FEMALE]);
    }
}

void MainWindow::setAttendenceTableItem(int row, int column, int number) {
    ui->tableAttendances->setItem(row, column, new QTableWidgetItem(QString::number(number)));
}

void MainWindow::setDiagnosisTableItem(int row, int column, int number) {
    ui->tableDiagnoses->setItem(row, column, new QTableWidgetItem(QString::number(number)));
}

void MainWindow::populateDiagnoses(int year, int month) {
    HMISData rows = db.fetchHMISData(year, month);

    // Copy initial attendance stats map
    auto map = diagnosisStats;

    for (HMISRow& row : rows) {
        for (QString& dx : row.diagnoses) {
            // This will insert missing diagnoses as per std::unordered_map.
            map[dx][row.ageCategory][row.sex]++;
        }
    }

    // Add stanadard diagnoses
    for (int row = 0; row < diagnosisNames.size(); row++) {
        QString dx = diagnosisNames[row];
        if (!map.contains(dx)) {
            continue;
        }

        setDiagnosisTableItem(row, 0, map[dx][ZERO_TO_TWENTY_EIGHT_DAYS][MALE]);
        setDiagnosisTableItem(row, 1, map[dx][ZERO_TO_TWENTY_EIGHT_DAYS][FEMALE]);

        setDiagnosisTableItem(row, 2, map[dx][TWENTY_NINE_DAYS_TO_FOUR_YEARS][MALE]);
        setDiagnosisTableItem(row, 3, map[dx][TWENTY_NINE_DAYS_TO_FOUR_YEARS][FEMALE]);

        setDiagnosisTableItem(row, 4, map[dx][FIVE_TO_NINE_YEARS][MALE]);
        setDiagnosisTableItem(row, 5, map[dx][FIVE_TO_NINE_YEARS][FEMALE]);

        setDiagnosisTableItem(row, 6, map[dx][TEN_TO_NINETEEN_YEARS][MALE]);
        setDiagnosisTableItem(row, 7, map[dx][TEN_TO_NINETEEN_YEARS][FEMALE]);

        setDiagnosisTableItem(row, 8, map[dx][TWENTY_YEARS_AND_ABOVE][MALE]);
        setDiagnosisTableItem(row, 9, map[dx][TWENTY_YEARS_AND_ABOVE][FEMALE]);
    }
}

void MainWindow::addToSelectedDiagnoses(QListWidgetItem* item) {
    QStringList selectedItems;
    for (int i = 0; i < ui->listWidgetSelected->count(); ++i) {
        selectedItems << ui->listWidgetSelected->item(i)->text();
    }

    int row = (int)selectedItems.size();
    if (selectedItems.contains(item->text())) {
        QMessageBox::information(this, "Duplicate diagnosis", item->text() + " already added!");
        return;
    }
    ui->listWidgetSelected->insertItem(row, item->text());
}

void MainWindow::removeFromSelectedDiagnoses(QListWidgetItem* item) {
    delete ui->listWidgetSelected->takeItem(ui->listWidgetSelected->row(item));
}

void MainWindow::onDateChanged(const QDate& date) {
    currentYear  = date.year();
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

void MainWindow::onExit() {
    qApp->quit();
}

void MainWindow::onViewRegister() {
    QDate date = ui->dateEdit->date();

    int year  = date.year();
    int month = date.month();

    auto* registerDialog = new Register(&db, year, month, this);

    registerDialog->setData(db.fetchHMISData(year, month));
    registerDialog->showMaximized();
}

void MainWindow::diagnosisQueryChanged(const QString& query) {
    filterDiagnoses(query);
}

void MainWindow::filterVisibleDiagnoses(const QString& query) {
    // match query with vertical header(this is the diagnosis name)
    int rowCount = ui->tableDiagnoses->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        QTableWidgetItem* item = ui->tableDiagnoses->verticalHeaderItem(row);

        if (item != nullptr) {
            QString text = item->text().trimmed();
            if (text.contains(query.trimmed(), Qt::CaseInsensitive)) {
                ui->tableDiagnoses->setRowHidden(row, false);
            } else {
                ui->tableDiagnoses->setRowHidden(row, true);
            }
        }
    }
}

// View all diagnoses in a modal dialog
void MainWindow::onViewDiagnoses() {
    auto* dialog = new QDialog(this);
    dialog->setWindowTitle("Diagnoses");
    dialog->setModal(true);
    dialog->setMinimumSize(600, 400);

    auto* layout = new QVBoxLayout(dialog);
    auto* list   = new QListWidget(dialog);
    list->addItems(diagnosisNames);

    layout->addWidget(list);
    dialog->setLayout(layout);

    dialog->exec();
}

// Open modal to add new diagnosis
void MainWindow::onAddDiagnosis() {
    auto* dialog = new QDialog(this);
    dialog->setWindowTitle("Add Diagnosis");
    dialog->setModal(true);
    dialog->setFixedSize(400, 200);

    auto* layout = new QVBoxLayout(dialog);
    auto* line   = new QLineEdit(dialog);
    line->setPlaceholderText("Enter diagnosis name");
    auto* button = new QPushButton("Add", dialog);

    layout->addWidget(line);
    layout->addWidget(button);

    connect(button, &QPushButton::clicked, [this, line, dialog]() {
        QString name = line->text().trimmed();
        if (name.isEmpty()) {
            QMessageBox::warning(this, "Error", "Diagnosis name cannot be empty");
            return;
        }

        if (db.diagnosisExists(name)) {
            QMessageBox::warning(this, "Error", "Diagnosis already exists");
            return;
        }

        if (db.insertDiagnoses(QStringList{name})) {
            diagnosisNames << name;
            ui->listWidgetAllDiagnoses->addItem(name);
            dialog->accept();
        } else {
            QMessageBox::critical(this, "Error", "Unable to add diagnosis");
        }
    });

    dialog->setLayout(layout);
    dialog->exec();
}
