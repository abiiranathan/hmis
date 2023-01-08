#include "register.h"

#include "ui_register.h"

Register::Register(QWidget *parent) : QDialog(parent), ui(new Ui::Register) {
  ui->setupUi(this);
  headers = {"Patient ID", "Age Category", "Sex", "New Attendance",
             "Diagnosis"};
}

Register::~Register() { delete ui; }

void Register::setData(const QList<HMISRow> &data) {
  this->data = data;
  this->filteredData = data;
  buildTableWidget();
  populateTableWithData();
}

void Register::on_search_textChanged(const QString &text) {
  if (text.trimmed().isEmpty()) {
    this->filteredData = data;
    populateTableWithData();
    return;
  }

  this->filteredData = {};
  for (HMISRow &row : data) {
    if (ui->comboBoxSearch->currentIndex() == 0) {
      // filter by patient ip number
      if (row.ipNumber == text) {
        filteredData << row;
      }
    } else {
      // filter by diagnosis
      for (QString &dx : row.diagnoses) {
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
  ui->tableWidget->setColumnCount(headers.size());

  QHeaderView *horizontalHeader = ui->tableWidget->horizontalHeader();
  horizontalHeader->setSectionResizeMode(0, QHeaderView::Fixed);
  horizontalHeader->setSectionResizeMode(1, QHeaderView::Fixed);
  horizontalHeader->setSectionResizeMode(2, QHeaderView::Fixed);
  horizontalHeader->setSectionResizeMode(3, QHeaderView::Fixed);
  horizontalHeader->setSectionResizeMode(4, QHeaderView::Stretch);

  horizontalHeader->setStyleSheet(
      "font-family: Arial; font-size: 12px; background-color: lightgray;");

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
  ui->tableWidget->setRowCount(filteredData.size());

  int row = 0;
  for (HMISRow &dataRow : filteredData) {
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(dataRow.ipNumber));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(dataRow.ageCategory));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(dataRow.sex));
    ui->tableWidget->setItem(row, 3,
                             new QTableWidgetItem(dataRow.newAttendance));
    ui->tableWidget->setItem(
        row, 4, new QTableWidgetItem(dataRow.diagnoses.join(", ")));
    row++;
  }

  ui->tableWidget->horizontalHeader()->resizeSections(
      QHeaderView::ResizeToContents);
  ui->tableWidget->resizeColumnsToContents();
}
