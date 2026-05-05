#include <QMainWindow>
#include <QMessageBox>
#include <algorithm>

#include "./ui_register.h"
#include "charts.hpp"
#include "register.hpp"

Register::Register(Database* db, int year, int month, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::Register), m_db(db), year(year), month(month) {
    ui->setupUi(this);
    setWindowTitle("HMIS Register");
    setMinimumSize(1200, 700);

    headers = {"ID", "Patient ID", "Age Category", "Sex", "New Attendance", "Diagnosis"};

    connect(ui->comboBoxSearch, &QComboBox::currentIndexChanged, this, &Register::onSearchTypeChanged);
    connect(ui->search, &QLineEdit::textChanged, this, &Register::onSearchTextChanged);
    connect(ui->tableWidget, &QTableWidget::itemChanged, this, &Register::itemChanged);
    connect(ui->btnDelete, &QPushButton::clicked, this, &Register::deleteSelectedRow);

    ui->search->setPlaceholderText("Search by Patient ID or Diagnosis");

    QString monthStr = month < 10 ? "0" + QString::number(month) : QString::number(month);
    ui->registerLabel->setText("HMIS REGISTER: " + monthStr + "/" + QString::number(year));
    ui->registerLabel->setStyleSheet("font-size:20px; font-weight:bold; color:blue;");

    itemChangeEnabled = false;
}

Register::~Register() {
    itemChangeEnabled = false;
    delete ui;
}

void Register::hideIDColumn() { ui->tableWidget->setColumnHidden(0, true); }

// ---------------------------------------------------------------------------
// Chart helpers using MonthlyStats
// ---------------------------------------------------------------------------
static QList<AbstractChart*> createDiagnosisCharts(const MonthlyStats& dxMap) {
    // Sort by total count descending, take top 10
    QVector<QPair<int, QString>> sorted;
    for (const QString& dx : dxMap.keys()) {
        int total = 0;
        for (const QString& age : AGE_CATEGORIES) total += dxMap.get(dx, age).total();
        if (total > 0) sorted << qMakePair(total, dx);
    }
    std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return a.first > b.first; });

    QList<AbstractChart*> charts;
    int count = 0;
    for (const auto& pair : sorted) {
        if (count++ >= 10) break;
        const QString& dx = pair.second;
        auto* chart = new BarChart(dx, QStringList(AGE_CATEGORIES.begin(), AGE_CATEGORIES.end()));
        bool allZero = true;
        for (const QString& sex : {SEX_MALE, SEX_FEMALE}) {
            std::vector<qreal> data;
            for (const QString& age : AGE_CATEGORIES) {
                auto cnt = dxMap.get(dx, age);
                data.push_back(sex == SEX_MALE ? cnt.male : cnt.female);
            }
            if (std::any_of(data.begin(), data.end(), [](qreal v) { return v > 0; })) {
                allZero = false;
                chart->addSeries(sex, data, (sex == SEX_MALE) ? QColor(Qt::blue) : QColor(Qt::red));
                qreal mx = *std::max_element(data.begin(), data.end());
                chart->setYRange(0, (mx * 1.2) + 1);
            }
        }

        if (!allZero)
            charts.append(chart);
        else
            delete chart;
    }
    return charts;
}

static QList<AbstractChart*> createAttendanceCharts(const MonthlyStats& attMap) {
    QList<AbstractChart*> charts;
    for (const QString& key : attMap.keys()) {
        auto* chart = new BarChart(key == ATT_YES ? "NEW ATTENDANCE" : "RE-ATTENDANCE",
                                   QStringList(AGE_CATEGORIES.begin(), AGE_CATEGORIES.end()));
        for (const QString& sex : {SEX_MALE, SEX_FEMALE}) {
            std::vector<qreal> data;
            qreal mx = 0;
            for (const QString& age : AGE_CATEGORIES) {
                auto cnt = attMap.get(key, age);
                qreal v = (sex == SEX_MALE) ? cnt.male : cnt.female;
                data.push_back(v);
                mx = std::max(mx, v);
            }
            chart->addSeries(sex, data, (sex == SEX_MALE) ? QColor(Qt::blue) : QColor(Qt::red));
            chart->setYRange(0, (mx * 1.2) + 1);
        }
        charts.append(chart);
    }
    return charts;
}

static void layoutCharts(Ui::Register* ui, const QList<AbstractChart*>& dxCharts,
                         const QList<AbstractChart*>& attCharts) {
    auto* main = new QVBoxLayout();
    main->addWidget(new QLabel("Attendance Charts"));
    for (auto* c : attCharts) {
        c->widget()->setMinimumSize(300, 400);
        main->addWidget(c->widget());
    }
    main->addWidget(new QLabel("Diagnosis Charts"));
    for (auto* c : dxCharts) {
        c->widget()->setMinimumSize(300, 400);
        main->addWidget(c->widget());
    }
    ui->chartLayout->addLayout(main, 0, 0);
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------
void Register::setData(const QList<HMISRow>& tableData) {
    data = filteredData = tableData;
    buildTableWidget();
    populateTableWithData();
}

void Register::plotData(const MonthlyStats& dxMap, const MonthlyStats& attendanceMap) {
    layoutCharts(ui, createDiagnosisCharts(dxMap), createAttendanceCharts(attendanceMap));
}

// ---------------------------------------------------------------------------
// Search
// ---------------------------------------------------------------------------
void Register::onSearchTypeChanged(int /*unused*/) {
    if (!ui->search->text().isEmpty()) onSearchTextChanged(ui->search->text());
}

void Register::onSearchTextChanged(const QString& text) {
    if (text.trimmed().isEmpty()) {
        filteredData = data;
        populateTableWithData();
        return;
    }
    filteredData.clear();
    if (ui->comboBoxSearch->currentIndex() == 0) {
        for (const HMISRow& row : data)
            if (row.ipNumber.startsWith(text)) filteredData << row;
    } else {
        for (const HMISRow& row : data)
            for (const QString& diag : row.diagnoses)
                if (diag.contains(text, Qt::CaseInsensitive)) {
                    filteredData << row;
                    break;
                }
    }
    populateTableWithData();
}

// ---------------------------------------------------------------------------
// Table
// ---------------------------------------------------------------------------
void Register::buildTableWidget() {
    ui->tableWidget->setColumnCount(static_cast<int>(headers.size()));
    QHeaderView* h = ui->tableWidget->horizontalHeader();
    for (int i = 0; i < 5; ++i) h->setSectionResizeMode(i, QHeaderView::Fixed);
    h->setSectionResizeMode(5, QHeaderView::Stretch);
    h->setStyleSheet("font-family:Arial; font-size:12px; background-color:lightgray;");
    ui->tableWidget->setStyleSheet(
        "QHeaderView::section { font-size:12px; }"
        "QHeaderView::section:nth-of-type(odd) { background-color:beige; color:purple; }");
    ui->tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked);
    ui->tableWidget->setAlternatingRowColors(true);
    ui->tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->tableWidget->setHorizontalHeaderLabels(headers);
}

static bool validateRowData(const QStringList& rowData, QString& error) {
    for (const auto& s : rowData)
        if (s.isEmpty()) {
            error = "Empty cell not allowed";
            return false;
        }
    if (rowData[3] != "Male" && rowData[3] != "Female") {
        error = "Sex must be Male or Female";
        return false;
    }
    if (rowData[4] != "YES" && rowData[4] != "NO") {
        error = "Attendance must be YES or NO";
        return false;
    }
    if (!AGE_CATEGORIES.contains(rowData[2])) {
        error = "Invalid age category";
        return false;
    }
    return true;
}

void Register::itemChanged(QTableWidgetItem* item) {
    if (!itemChangeEnabled || item == nullptr) return;

    int row = item->row();
    if (item->column() == 0) return;

    QStringList rowData;
    for (int col = 0; col < ui->tableWidget->columnCount(); ++col) {
        auto* cell = ui->tableWidget->item(row, col);
        rowData << (cell != nullptr ? cell->text() : "");
    }

    QString error;
    if (!validateRowData(rowData, error)) {
        QMessageBox::warning(this, "Validation Error", error);
        return;
    }

    HMISRow hmisRow = {
        .id = rowData[0].toInt(),
        .ageCategory = rowData[2],
        .sex = rowData[3],
        .newAttendance = rowData[4],
        .diagnoses = rowData[5].split(","),
        .ipNumber = rowData[1],
        .year = 0,
        .month = 0,
    };

    for (const QString& diag : hmisRow.diagnoses)
        if (!m_db->diagnosisExists(diag.trimmed())) m_db->insertDiagnoses(QStringList{diag.trimmed()});

    if (!m_db->updateHMISRow(hmisRow, m_currentUser.id))
        QMessageBox::warning(this, "Update Error", "Update failed: " + m_db->getLastError());
}

void Register::populateTableWithData() {
    itemChangeEnabled = false;
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(static_cast<int>(filteredData.size()));

    for (int row = 0; row < static_cast<int>(filteredData.size()); ++row) {
        const HMISRow& r = filteredData[row];
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(r.id)));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(r.ipNumber));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(r.ageCategory));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(r.sex));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(r.newAttendance));
        ui->tableWidget->setItem(row, 5, new QTableWidgetItem(r.diagnoses.join(", ")));
    }

    ui->tableWidget->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    ui->tableWidget->resizeColumnsToContents();
    hideIDColumn();
    itemChangeEnabled = true;
}

void Register::deleteSelectedRow() {
    if (QMessageBox::question(this, "Delete Row", "Delete the selected record?", QMessageBox::Yes | QMessageBox::No) ==
        QMessageBox::No)
        return;

    auto selected = ui->tableWidget->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected[0]->row();
    auto* idItem = ui->tableWidget->item(row, 0);
    if (idItem == nullptr) return;

    int id = idItem->text().toInt();
    if (id == 0) {
        QMessageBox::warning(this, "Error", "Invalid ID");
        return;
    }

    if (m_db->deleteHMISRow(id, m_currentUser.id))
        ui->tableWidget->removeRow(row);
    else
        QMessageBox::critical(this, "Error", "Delete failed: " + m_db->getLastError());
}
