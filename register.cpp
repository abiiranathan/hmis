#include "register.h"

#include "ui_register.h"

Register::Register(QWidget* parent) : QDialog(parent), ui(new Ui::Register) {
    ui->setupUi(this);
    this->setMinimumSize(QSize(1200, 700));
    headers = {"Patient ID", "Age Category", "Sex", "New Attendance", "Diagnosis"};

    connect(ui->comboBoxSearch, &QComboBox::currentIndexChanged, this, &Register::onSearchTypeChanged);
    connect(ui->search, &QLineEdit::textChanged, this, &Register::onSearchTextChanged);
}

Register::~Register() {
    delete ui;
}

void Register::setData(const QList<HMISRow>& tableData) {
    this->data         = tableData;
    this->filteredData = tableData;
    buildTableWidget();
    populateTableWithData();
}

void Register::onSearchTypeChanged(int) {
    auto text = ui->search->text();
    if (!text.isEmpty()) {
        onSearchTextChanged(text);
    }
}

void Register::onSearchTextChanged(const QString& text) {
    if (text.trimmed().isEmpty()) {
        this->filteredData = data;
        populateTableWithData();
        return;
    }

    this->filteredData = {};

    // filter by patient ip number
    if (ui->comboBoxSearch->currentIndex() == 0) {
        for (HMISRow& row : data) {
            if (row.ipNumber.startsWith(text)) {
                filteredData << row;
            }
        }
    } else {
        // filter by diagnosis
        for (HMISRow& row : data) {
            for (QString& dx : row.diagnoses) {
                if (dx.contains(text, Qt::CaseInsensitive)) {
                    filteredData << row;
                    break;
                }
            }
        }
    }

    populateTableWithData();
}

void Register::buildTableWidget() {
    ui->tableWidget->setColumnCount((int)headers.size());

    QHeaderView* horizontalHeader = ui->tableWidget->horizontalHeader();
    horizontalHeader->setSectionResizeMode(0, QHeaderView::Fixed);
    horizontalHeader->setSectionResizeMode(1, QHeaderView::Fixed);
    horizontalHeader->setSectionResizeMode(2, QHeaderView::Fixed);
    horizontalHeader->setSectionResizeMode(3, QHeaderView::Fixed);
    horizontalHeader->setSectionResizeMode(4, QHeaderView::Stretch);

    horizontalHeader->setStyleSheet("font-family: Arial; font-size: 12px; background-color: lightgray;");

    ui->tableWidget->setStyleSheet(
        "QHeaderView::section {font-size:12px; } "
        "QHeaderView::section:nth-of-type(odd) { "
        "background-color: beige; color:purple; }");

    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setAlternatingRowColors(true);
    ui->tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->tableWidget->setHorizontalHeaderLabels(headers);
}

void Register::populateTableWithData() {
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount((int)filteredData.size());

    int row = 0;
    for (HMISRow& dataRow : filteredData) {
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(dataRow.ipNumber));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(dataRow.ageCategory));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(dataRow.sex));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(dataRow.newAttendance));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(dataRow.diagnoses.join(", ")));
        row++;
    }

    ui->tableWidget->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    ui->tableWidget->resizeColumnsToContents();
}
