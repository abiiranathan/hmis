#include <QMainWindow>
#include <QMessageBox>
#include <QTableView>

#include "./ui_register.h"

#include "register.hpp"

Register::Register(Database* db, int year, int month, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::Register), m_db(db), year(year), month(month) {
    ui->setupUi(this);
    this->setWindowTitle("HMIS Register");
    this->setMinimumSize(QSize(1200, 700));
    headers = {"ID", "Patient ID", "Age Category", "Sex", "New Attendance", "Diagnosis"};

    connect(ui->comboBoxSearch, &QComboBox::currentIndexChanged, this, &Register::onSearchTypeChanged);
    connect(ui->search, &QLineEdit::textChanged, this, &Register::onSearchTextChanged);
    connect(ui->tableWidget, &QTableWidget::itemChanged, this, &Register::itemChanged);
    connect(ui->btnDelete, &QPushButton::clicked, this, &Register::deleteSelectedRow);
    ui->search->setPlaceholderText("Search by Patient ID or Diagnosis");

    // format month with leading zero
    QString monthStr = QString::number(month);
    if (month < 10) {
        monthStr = "0" + monthStr;
    }

    ui->registerLabel->setText("HMIS REGISTER: " + monthStr + "/" + QString::number(year));
    ui->registerLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: blue;");
    itemChangeEnabled = false;
}

Register::~Register() {
    itemChangeEnabled = false;
    delete ui;
}

void Register::hideIDColumn() {
    ui->tableWidget->setColumnHidden(0, true);
}

void Register::disableIDColumn() {
    // Disable editing of ID column
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
        QTableWidgetItem* item = ui->tableWidget->item(row, 0);
        if (item != nullptr) {
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        }
    }
}

void Register::setData(const QList<HMISRow>& tableData) {
    this->data         = tableData;
    this->filteredData = tableData;

    buildTableWidget();
    populateTableWithData();

    // TODO: Display a chart in ui->chartLayout (a QGridLayout)
    // TODO: Implement chart display
    // TODO: Accept aggregated chart data as a parameter.
    // This is because the data is already aggregate in mainWindow.cpp
}

void Register::onSearchTypeChanged(int /*unused*/) {
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
            for (QString& diag : row.diagnoses) {
                if (diag.contains(text, Qt::CaseInsensitive)) {
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
    horizontalHeader->setSectionResizeMode(4, QHeaderView::Fixed);
    horizontalHeader->setSectionResizeMode(5, QHeaderView::Stretch);

    horizontalHeader->setStyleSheet("font-family: Arial; font-size: 12px; background-color: lightgray;");

    ui->tableWidget->setStyleSheet(
        "QHeaderView::section {font-size:12px; } "
        "QHeaderView::section:nth-of-type(odd) { "
        "background-color: beige; color:purple; }");

    // Allow editing via double click.
    ui->tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked);
    ui->tableWidget->setAlternatingRowColors(true);
    ui->tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->tableWidget->setHorizontalHeaderLabels(headers);
}

static bool validateRowData(const QStringList& rowData, QString& error) {
    //  headers = {"ID", "Patient ID", "Age Category", "Sex", "New Attendance", "Diagnosis"};
    for (const auto& str : rowData) {
        if (str.isEmpty()) {
            error = "Empty cell not allowed";
            return false;
        }
    }

    // Validate sex
    if (rowData[3] != "Male" && rowData[3] != "Female") {
        error = "Invalid sex. Must be Male or Female";
        return false;
    }

    // validate new attendance
    if (rowData[4] != "YES" && rowData[4] != "NO") {
        error = "Invalid attendance status. Must be YES or NO";
        return false;
    }

    // Validate age category
    static const QStringList ageCategories = {
        "0 - 28 days",
        "29 days - 4 years",
        "5 - 9 years",
        "10 - 19 years",
        "20 years and above",
    };

    if (!ageCategories.contains(rowData[2])) {
        error = "Invalid age Category. Must be one of " + ageCategories.join(", ");
        return false;
    }

    return true;
}

void Register::itemChanged(QTableWidgetItem* item) {
    if (!itemChangeEnabled) {
        return;
    }

    auto showError = [&](const QString& msg) {
        QMessageBox::warning(this, "Validation Error", msg);
    };

    if (item != nullptr) {
        // Get the row of the changed item
        int row    = item->row();
        int column = item->column();

        // Ignore ID column
        if (column == 0) {
            return;
        }

        int columnCount = ui->tableWidget->columnCount();

        QStringList rowData;

        for (int col = 0; col < columnCount; ++col) {
            QTableWidgetItem* cellItem = ui->tableWidget->item(row, col);
            if (cellItem != nullptr) {
                rowData.append(cellItem->text());
            }
        }

        QString error;
        if (!validateRowData(rowData, error)) {
            showError(error);
            return;
        }

        // Add diagnoses to file if they don't exit
        const HMISRow& hmisRow = {
            .id            = rowData[0].toInt(),
            .ageCategory   = rowData[2],
            .sex           = rowData[3],
            .newAttendance = rowData[4],
            .diagnoses     = rowData[5].split(","),
            .ipNumber      = rowData[1],
            .year          = 0,  // we are not updating the year
            .month         = 0,  // we are not updating the month
        };

        for (const auto& diag : hmisRow.diagnoses) {
            if (!m_db->diagnosisExists(diag)) {
                m_db->insertDiagnoses(QStringList{diag});
                if (m_db->getLastError() != "") {
                    showError("Error inserting diagnosis: " + diag);
                    return;
                }
            }
        }

        if (!m_db->updateHMISRow(hmisRow)) {
            showError("Error updating row: " + m_db->getLastError());
        }
    }
}

void Register::populateTableWithData() {
    itemChangeEnabled = false;  // disable itemChanged signal during populating of table.

    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount((int)filteredData.size());

    int row = 0;
    for (HMISRow& dataRow : filteredData) {
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(dataRow.id)));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(dataRow.ipNumber));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(dataRow.ageCategory));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(dataRow.sex));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(dataRow.newAttendance));
        ui->tableWidget->setItem(row, 5, new QTableWidgetItem(dataRow.diagnoses.join(", ")));
        row++;
    }

    ui->tableWidget->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    ui->tableWidget->resizeColumnsToContents();

    hideIDColumn();
    itemChangeEnabled = true;  // re-enable signal.
}

void Register::deleteSelectedRow() {
    // Get confirmation from user
    if (QMessageBox::question(this,
                              "Delete Row",
                              "Are you sure you want to delete the selected row?",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
        return;
    }

    // Get selected row
    auto selectedItems = ui->tableWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    // Get the row of the selected item
    int row = selectedItems[0]->row();

    // Get the ID of the selected row
    int id = ui->tableWidget->item(row, 0)->text().toInt();
    if (id == 0) {
        QMessageBox::warning(this, "Error", "Invalid ID");
        return;
    }

    // Delete the row from the database
    if (m_db->deleteHMISRow(id)) {
        ui->tableWidget->removeRow(row);
        QMessageBox::information(this, "Success", "Row deleted successfully");
    } else {
        QMessageBox::critical(this, "Error", "Unable to delete row: " + m_db->getLastError());
    }
}
