#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include "./ui_mainwindow.h"

#include "AuditLogDialog.hpp"
#include "mainwindow.hpp"
#include "register.hpp"

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------
MainWindow::MainWindow(Database& conn, const User& user, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), db(conn), m_currentUser(user) {
    ui->setupUi(this);
    setWindowIcon(QIcon(":/favicon.ico"));
    setWindowTitle(
        QString("HMIS 105  —  %1 [%2]").arg(user.username).arg(user.role == UserRole::Admin ? "Admin" : "Clerk"));
    menuBar()->hide();

    connectSignals();
    initUI();

    QDate date = QDate::currentDate();
    ui->dateEdit->setDate(date);
    ui->dateEdit->setMaximumDate(QDate::currentDate());

    QSettings settings;
    restoreGeometry(settings.value("mainwindow/geometry").toByteArray());
}

MainWindow::~MainWindow() {
    QSettings settings;
    settings.setValue("mainwindow/geometry", saveGeometry());
    delete ui;
}

// ---------------------------------------------------------------------------
// Default diagnoses from resource file
// ---------------------------------------------------------------------------
static QStringList readDefaultDiagnoses(QWidget* parent) {
    QFile file(":/diagnoses.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(parent, "Error", "Unable to open diagnoses file");
        return {};
    }
    QStringList out;
    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (!line.isEmpty()) {
            out << line;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// initUI
// ---------------------------------------------------------------------------
void MainWindow::initUI() {
    auto storedDiagnoses = db.getAllDiagnoses();
    if (!storedDiagnoses) {
        QMessageBox::critical(this, "Error", "Unable to fetch diagnoses from the database");
        qApp->exit(1);
        return;
    }

    diagnoses = storedDiagnoses.value();
    if (diagnoses.isEmpty()) {
        if (!db.insertDiagnoses(readDefaultDiagnoses(this))) {
            QMessageBox::critical(this, "Error", "Unable to insert default diagnoses");
            qApp->exit(1);
            return;
        }
        storedDiagnoses = db.getAllDiagnoses();
        if (!storedDiagnoses || storedDiagnoses->isEmpty()) {
            QMessageBox::critical(this, "Error", "No diagnoses found");
            qApp->exit(1);
            return;
        }
        diagnoses = storedDiagnoses.value();
    }

    diagnosisNames.clear();
    for (const Diagnosis& d : diagnoses) {
        diagnosisNames << d.name;
    }

    ui->listWidgetAllDiagnoses->clear();
    ui->listWidgetAllDiagnoses->addItems(diagnosisNames);

    initializeTableWidget(ui->tableAttendances, 2);
    ui->tableAttendances->setStyleSheet(
        "QHeaderView::section { font-size:12px; }"
        "QHeaderView::section:nth-of-type(odd) { background-color:beige; color:purple; }");
    ui->tableAttendances->setMaximumHeight(85);

    initializeTableWidget(ui->tableDiagnoses, static_cast<int>(diagnosisNames.size()));
    ui->tableDiagnoses->setStyleSheet("QHeaderView::section { font-size:14px; background-color:white; color:black; }");
    ui->tableDiagnoses->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { font-size:11px; background-color:beige; color:purple; }");

    ui->tableAttendances->setVerticalHeaderLabels({"NEW ATTENDANCE", "RE-ATTENDANCE"});
    ui->tableDiagnoses->setVerticalHeaderLabels(diagnosisNames);
    ui->comboBoxCategory->setCurrentText(AGE_20_PLUS);
}

// ---------------------------------------------------------------------------
// connectSignals
// ---------------------------------------------------------------------------
void MainWindow::connectSignals() {
    connect(ui->listWidgetAllDiagnoses, &QListWidget::itemDoubleClicked, this, &MainWindow::addToSelectedDiagnoses);
    connect(ui->listWidgetSelected, &QListWidget::itemDoubleClicked, this, &MainWindow::removeFromSelectedDiagnoses);
    connect(ui->lineEdit, &QLineEdit::textChanged, this, &MainWindow::diagnosisQueryChanged);
    connect(ui->dateEdit, &QDateEdit::dateChanged, this, &MainWindow::onDateChanged);
    connect(ui->btnSave, &QPushButton::clicked, this, &MainWindow::onSave);
    connect(ui->btnReset, &QPushButton::clicked, this, &MainWindow::onResetForm);

    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onExit);
    connect(ui->actionExpand, &QAction::triggered, this, &MainWindow::onToggleSidebar);
    connect(ui->actionView_Register, &QAction::triggered, this, &MainWindow::onViewRegister);
    connect(ui->checkHideEmpty, &QCheckBox::checkStateChanged, this, &MainWindow::toggleHideEmptyDiagnoses);
    connect(ui->txtFilter, &QLineEdit::textChanged, this, &MainWindow::filterVisibleDiagnoses);
    connect(ui->actionView_All_Diagnoses, &QAction::triggered, this, &MainWindow::onViewDiagnoses);
    connect(ui->actionRegister_New_Diagnosis, &QAction::triggered, this, &MainWindow::onAddDiagnosis);
    connect(ui->actionExport_CSV, &QAction::triggered, this, &MainWindow::onExportCSV);
    connect(ui->actionBackup_Database, &QAction::triggered, this, &MainWindow::onBackupDatabase);
    connect(ui->actionAudit_Log, &QAction::triggered, this, &MainWindow::onViewAuditLog);
    connect(ui->actionManage_Users, &QAction::triggered, this, &MainWindow::onManageUsers);
    connect(ui->actionChange_Password, &QAction::triggered, this, &MainWindow::onChangePassword);

    // Hide admin-only actions from clerks
    if (m_currentUser.role != UserRole::Admin) {
        ui->actionAudit_Log->setVisible(false);
        ui->actionManage_Users->setVisible(false);
        ui->actionBackup_Database->setVisible(false);
    }
}

// ---------------------------------------------------------------------------
// Form validation
// ---------------------------------------------------------------------------
QStringList MainWindow::validateForm() const {
    QStringList errors;
    if (ui->IPN->text().trimmed().isEmpty()) {
        errors << "IP Number is required.";
    }
    if (ui->comboBoxCategory->currentText().trimmed().isEmpty()) {
        errors << "Age category is required.";
    }
    if (ui->comboBoxSex->currentText().trimmed().isEmpty()) {
        errors << "Sex is required.";
    }
    return errors;
}

// ---------------------------------------------------------------------------
// Save
// ---------------------------------------------------------------------------
void MainWindow::onSave() {
    QStringList errors = validateForm();
    if (!errors.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please fix the following:\n\n• " + errors.join("\n• "));
        return;
    }

    QDate d = ui->dateEdit->date();
    QString ipNum = ui->IPN->text().trimmed();

    QStringList dxList;
    for (int i = 0; i < ui->listWidgetSelected->count(); ++i) {
        dxList << ui->listWidgetSelected->item(i)->text();
    }

    NewHMISData data = {
        .ageCategory = ui->comboBoxCategory->currentText(),
        .sex = ui->comboBoxSex->currentText(),
        .newAttendance = ui->comboBoxNewAttendance->currentText(),
        .diagnoses = dxList,
        .ipNumber = ipNum,
        .month = d.month(),
        .year = d.year(),
    };

    if (db.saveNewRow(data, m_currentUser.id)) {
        onResetForm();
        populateAttendances(d.year(), d.month());
        populateDiagnoses(d.year(), d.month());
        updateDashboard(d.year(), d.month());
        statusBar()->showMessage("Record inserted successfully", 5000);

        bool ok;
        int n = ipNum.toInt(&ok);
        if (ok) {
            ui->IPN->setText(QString("%1").arg(n + 1, 3, 10, QChar('0')));
        }
    } else {
        QMessageBox::critical(this, "Insert Error", "Unable to insert record:\n" + db.getLastError());
    }
}

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------
void MainWindow::onResetForm() {
    if (ui->noClearForm->isChecked()) {
        return;
    }
    if (!ui->checkKeepDiagnoses->isChecked()) {
        ui->listWidgetSelected->clear();
    }
    ui->comboBoxCategory->setCurrentText(AGE_20_PLUS);
    ui->comboBoxNewAttendance->setCurrentText(ATT_YES);
    ui->comboBoxSex->setCurrentText(SEX_FEMALE);
}

// ---------------------------------------------------------------------------
// Table helpers
// ---------------------------------------------------------------------------
void MainWindow::initializeTableWidget(QTableWidget* w, int rowCount) {
    w->setColumnCount(static_cast<int>(diagnosisTableHeaders.size()));
    w->setHorizontalHeaderLabels(diagnosisTableHeaders);
    QHeaderView* h = w->horizontalHeader();
    for (int i = 0; i < static_cast<int>(diagnosisTableHeaders.size()); ++i) {
        h->setSectionResizeMode(i, QHeaderView::Stretch);
    }
    h->setStyleSheet("font-family:Arial; font-size:12px; background-color:lightgray;");
    w->setEditTriggers(QAbstractItemView::NoEditTriggers);
    w->setAlternatingRowColors(true);
    w->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    w->setRowCount(rowCount);
}

void MainWindow::setAttendanceTableItem(int row, int col, int number) {
    auto* item = new QTableWidgetItem(QString::number(number));
    if (number > 0) {
        item->setFont(QFont("Arial", 12, QFont::Bold));
    }
    ui->tableAttendances->setItem(row, col, item);
}

void MainWindow::setDiagnosisTableItem(int row, int col, int number) {
    auto* item = new QTableWidgetItem(QString::number(number));
    if (number > 0) {
        item->setFont(QFont("Arial", 12, QFont::Bold));
    }
    ui->tableDiagnoses->setItem(row, col, item);
}

// ---------------------------------------------------------------------------
// Populate tables
// ---------------------------------------------------------------------------
void MainWindow::populateAttendances(int year, int month) {
    HMISData rows = db.fetchHMISData(year, month);
    MonthlyStats st = db.buildAttendanceStats(rows);

    for (int r = 0; r < 2; r++) {
        QString att = (r == 0) ? ATT_YES : ATT_NO;
        int col = 0;
        for (const QString& age : AGE_CATEGORIES) {
            auto cnt = st.get(att, age);
            setAttendanceTableItem(r, col++, cnt.male);
            setAttendanceTableItem(r, col++, cnt.female);
        }
    }
}

void MainWindow::populateDiagnoses(int year, int month) {
    HMISData rows = db.fetchHMISData(year, month);
    MonthlyStats st = db.buildDiagnosisStats(rows, diagnosisNames);

    for (int row = 0; row < static_cast<int>(diagnosisNames.size()); row++) {
        int col = 0;
        for (const QString& age : AGE_CATEGORIES) {
            auto cnt = st.get(diagnosisNames[row], age);
            setDiagnosisTableItem(row, col++, cnt.male);
            setDiagnosisTableItem(row, col++, cnt.female);
        }
    }
}

// ---------------------------------------------------------------------------
// Dashboard summary
// ---------------------------------------------------------------------------
void MainWindow::updateDashboard(int year, int month) {
    auto s = db.getMonthlySummary(year, month);
    QString msg =
        QString("Total: %1  |  New: %2  |  Re-att: %3").arg(s.totalPatients).arg(s.newAttendances).arg(s.reAttendances);
    if (!s.topDiagnosis1.isEmpty()) {
        msg += "  |  Top: " + s.topDiagnosis1;
    }
    if (!s.topDiagnosis2.isEmpty()) {
        msg += ", " + s.topDiagnosis2;
    }
    if (!s.topDiagnosis3.isEmpty()) {
        msg += ", " + s.topDiagnosis3;
    }
    statusBar()->showMessage(msg);
}

// ---------------------------------------------------------------------------
// Date change
// ---------------------------------------------------------------------------
void MainWindow::onDateChanged(const QDate& date) {
    currentYear = date.year();
    currentMonth = date.month();
    populateAttendances(currentYear, currentMonth);
    populateDiagnoses(currentYear, currentMonth);
    updateDashboard(currentYear, currentMonth);
    ui->IPN->setText(db.nextIPNumber(date.year(), date.month()));
}

// ---------------------------------------------------------------------------
// Diagnoses list
// ---------------------------------------------------------------------------
void MainWindow::addToSelectedDiagnoses(QListWidgetItem* item) {
    for (int i = 0; i < ui->listWidgetSelected->count(); ++i) {
        if (ui->listWidgetSelected->item(i)->text() == item->text()) {
            QMessageBox::information(this, "Duplicate", item->text() + " already added.");
            return;
        }
    }
    ui->listWidgetSelected->addItem(item->text());
}

void MainWindow::removeFromSelectedDiagnoses(QListWidgetItem* item) {
    delete ui->listWidgetSelected->takeItem(ui->listWidgetSelected->row(item));
}

void MainWindow::diagnosisQueryChanged(const QString& query) { filterDiagnoses(query); }

void MainWindow::filterDiagnoses(const QString& query) {
    ui->listWidgetAllDiagnoses->clear();
    if (query.trimmed().isEmpty()) {
        ui->listWidgetAllDiagnoses->addItems(diagnosisNames);
        return;
    }
    for (const QString& d : diagnosisNames) {
        if (d.contains(query, Qt::CaseInsensitive)) {
            ui->listWidgetAllDiagnoses->addItem(d);
        }
    }
}

void MainWindow::toggleHideEmptyDiagnoses(Qt::CheckState state) {
    int cols = ui->tableDiagnoses->columnCount();
    for (int row = 0; row < ui->tableDiagnoses->rowCount(); ++row) {
        bool empty = true;
        for (int col = 0; col < cols; ++col) {
            if (ui->tableDiagnoses->model()->data(ui->tableDiagnoses->model()->index(row, col)).toInt() != 0) {
                empty = false;
                break;
            }
        }
        ui->tableDiagnoses->setRowHidden(row, state == Qt::Checked && empty);
    }
}

void MainWindow::filterVisibleDiagnoses(const QString& query) {
    for (int row = 0; row < ui->tableDiagnoses->rowCount(); ++row) {
        auto* item = ui->tableDiagnoses->verticalHeaderItem(row);
        if (item != nullptr) {
            ui->tableDiagnoses->setRowHidden(row, !item->text().contains(query.trimmed(), Qt::CaseInsensitive));
        }
    }
}

// ---------------------------------------------------------------------------
// Sidebar
// ---------------------------------------------------------------------------
void MainWindow::onToggleSidebar(bool toggled) {
    ui->sidebar->setVisible(toggled);
    ui->actionExpand->setIcon(QIcon(toggled ? ":/icons/toggle-on.png" : ":/icons/toggle-off.png"));
}

// ---------------------------------------------------------------------------
// View register
// ---------------------------------------------------------------------------
void MainWindow::onViewRegister() {
    QDate date = ui->dateEdit->date();
    HMISData rows = db.fetchHMISData(date.year(), date.month());
    auto* reg = new Register(&db, date.year(), date.month(), this);
    reg->setCurrentUser(m_currentUser);
    reg->setData(rows);
    reg->showMaximized();
    reg->plotData(db.buildDiagnosisStats(rows, diagnosisNames), db.buildAttendanceStats(rows));
}

// ---------------------------------------------------------------------------
// Diagnoses management
// ---------------------------------------------------------------------------
void MainWindow::onViewDiagnoses() {
    auto* dialog = new QDialog(this);
    dialog->setWindowTitle("All Diagnoses");
    dialog->setModal(true);
    dialog->setMinimumSize(600, 400);
    auto* layout = new QVBoxLayout(dialog);
    auto* list = new QListWidget(dialog);
    list->addItems(diagnosisNames);
    layout->addWidget(list);
    dialog->exec();
}

void MainWindow::onAddDiagnosis() {
    auto* dialog = new QDialog(this);
    dialog->setWindowTitle("Register New Diagnosis");
    dialog->setModal(true);
    dialog->setFixedSize(420, 140);
    auto* layout = new QVBoxLayout(dialog);
    auto* line = new QLineEdit(dialog);
    line->setPlaceholderText("Enter diagnosis name");
    auto* btn = new QPushButton("Add", dialog);
    layout->addWidget(line);
    layout->addWidget(btn);
    connect(btn, &QPushButton::clicked, [this, line, dialog]() {
        QString name = line->text().trimmed();
        if (name.isEmpty()) {
            QMessageBox::warning(this, "Error", "Name cannot be empty.");
            return;
        }
        if (db.diagnosisExists(name)) {
            QMessageBox::warning(this, "Duplicate", "Already exists.");
            return;
        }
        if (db.insertDiagnoses(QStringList{name})) {
            diagnosisNames << name;
            ui->listWidgetAllDiagnoses->addItem(name);
            int newRow = ui->tableDiagnoses->rowCount();
            ui->tableDiagnoses->setRowCount(newRow + 1);
            ui->tableDiagnoses->setVerticalHeaderItem(newRow, new QTableWidgetItem(name));
            for (int c = 0; c < static_cast<int>(diagnosisTableHeaders.size()); ++c) {
                ui->tableDiagnoses->setItem(newRow, c, new QTableWidgetItem("0"));
            }
            dialog->accept();
        } else {
            QMessageBox::critical(this, "Error", "Failed: " + db.getLastError());
        }
    });
    dialog->exec();
}

// ---------------------------------------------------------------------------
// Export CSV
// ---------------------------------------------------------------------------
void MainWindow::onExportCSV() {
    QDate date = ui->dateEdit->date();
    QString def = QString("HMIS_%1_%2.csv").arg(date.year()).arg(date.month());
    QString path = QFileDialog::getSaveFileName(this, "Export CSV", def, "CSV Files (*.csv);;All Files (*)");
    if (path.isEmpty()) {
        return;
    }

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Export Error", "Cannot write to: " + path);
        return;
    }
    f.write(db.exportCSV(date.year(), date.month()).toUtf8());
    f.close();
    statusBar()->showMessage("Exported to " + path, 5000);
}

// ---------------------------------------------------------------------------
// Backup
// ---------------------------------------------------------------------------
void MainWindow::onBackupDatabase() {
    QString path = QFileDialog::getSaveFileName(this, "Backup Database", "hmis_backup.sqlite3",
                                                "SQLite (*.sqlite3);;All Files (*)");
    if (path.isEmpty()) {
        return;
    }
    if (db.backupTo(path)) {
        QMessageBox::information(this, "Backup Complete", "Backed up to:\n" + path);
    } else {
        QMessageBox::critical(this, "Backup Failed", "Backup failed. Only SQLite databases can be backed up this way.");
    }
}

// ---------------------------------------------------------------------------
// Audit log
// ---------------------------------------------------------------------------
void MainWindow::onViewAuditLog() {
    AuditLogDialog dlg(db, this);
    dlg.exec();
}

// ---------------------------------------------------------------------------
// User management
// ---------------------------------------------------------------------------
void MainWindow::onManageUsers() {
    auto* dialog = new QDialog(this);
    dialog->setWindowTitle("Manage Users");
    dialog->setMinimumSize(520, 400);
    auto* vl = new QVBoxLayout(dialog);

    auto* table = new QTableWidget(dialog);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({"ID", "Username", "Role"});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    vl->addWidget(table);

    auto loadUsers = [&]() {
        auto users = db.getAllUsers();
        table->setRowCount(static_cast<int>(users.size()));
        for (int i = 0; i < static_cast<int>(users.size()); ++i) {
            table->setItem(i, 0, new QTableWidgetItem(QString::number(users[i].id)));
            table->setItem(i, 1, new QTableWidgetItem(users[i].username));
            table->setItem(i, 2, new QTableWidgetItem(users[i].role == UserRole::Admin ? "Admin" : "Clerk"));
        }
    };
    loadUsers();

    auto* addGroup = new QGroupBox("Add New User", dialog);
    auto* fl = new QFormLayout(addGroup);
    auto* newUser = new QLineEdit(addGroup);
    auto* newPass = new QLineEdit(addGroup);
    newPass->setEchoMode(QLineEdit::Password);
    auto* roleCombo = new QComboBox(addGroup);
    roleCombo->addItems({"Clerk", "Admin"});
    fl->addRow("Username:", newUser);
    fl->addRow("Password:", newPass);
    fl->addRow("Role:", roleCombo);
    auto* addBtn = new QPushButton("Add User", addGroup);
    fl->addRow(addBtn);
    vl->addWidget(addGroup);

    connect(addBtn, &QPushButton::clicked, [&]() {
        QString u = newUser->text().trimmed(), p = newPass->text();
        if (u.isEmpty() || p.isEmpty()) {
            QMessageBox::warning(dialog, "Error", "Username and password required.");
            return;
        }
        if (db.userExists(u)) {
            QMessageBox::warning(dialog, "Error", "Username already taken.");
            return;
        }
        UserRole r = roleCombo->currentText() == "Admin" ? UserRole::Admin : UserRole::Clerk;
        if (db.createUser(u, p, r)) {
            loadUsers();
            newUser->clear();
            newPass->clear();
        } else {
            QMessageBox::critical(dialog, "Error", "Failed to create user.");
        }
    });

    auto* closeBtn = new QPushButton("Close", dialog);
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    vl->addWidget(closeBtn);
    dialog->exec();
}

// ---------------------------------------------------------------------------
// Change password
// ---------------------------------------------------------------------------
void MainWindow::onChangePassword() {
    auto* dialog = new QDialog(this);
    dialog->setWindowTitle("Change Password");
    dialog->setFixedSize(340, 210);
    auto* fl = new QFormLayout(dialog);
    auto* current = new QLineEdit(dialog);
    current->setEchoMode(QLineEdit::Password);
    auto* newPw = new QLineEdit(dialog);
    newPw->setEchoMode(QLineEdit::Password);
    auto* confirm = new QLineEdit(dialog);
    confirm->setEchoMode(QLineEdit::Password);
    fl->addRow("Current password:", current);
    fl->addRow("New password:", newPw);
    fl->addRow("Confirm:", confirm);
    auto* btn = new QPushButton("Update Password", dialog);
    fl->addRow(btn);
    connect(btn, &QPushButton::clicked, [&]() {
        if (newPw->text() != confirm->text()) {
            QMessageBox::warning(dialog, "Mismatch", "Passwords don't match.");
            return;
        }
        if (newPw->text().length() < 6) {
            QMessageBox::warning(dialog, "Too short", "Minimum 6 characters.");
            return;
        }
        if (!db.authenticate(m_currentUser.username, current->text())) {
            QMessageBox::warning(dialog, "Error", "Current password incorrect.");
            return;
        }
        if (db.changePassword(m_currentUser.id, newPw->text())) {
            QMessageBox::information(dialog, "Success", "Password updated.");
        } else {
            QMessageBox::critical(dialog, "Error", "Update failed.");
        }
        dialog->accept();
    });
    dialog->exec();
}

// ---------------------------------------------------------------------------
// Exit
// ---------------------------------------------------------------------------
void MainWindow::onExit() { qApp->quit(); }
