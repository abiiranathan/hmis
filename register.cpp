#include "register.h"
#include "ui_register.h"

#include <QMessageBox>

Register::Register(Database *db, QWidget *parent)
    : QDialog(parent), m_db(db), ui(new Ui::Register) {
  ui->setupUi(this);
  this->setMinimumSize(QSize(1200, 700));
  headers = {"ID",  "Patient ID",     "Age Category",
             "Sex", "New Attendance", "Diagnosis"};

  connect(ui->comboBoxSearch, &QComboBox::currentIndexChanged, this,
          &Register::onSearchTypeChanged);
  connect(ui->search, &QLineEdit::textChanged, this,
          &Register::onSearchTextChanged);
  connect(ui->tableWidget, &QTableWidget::itemChanged, this,
          &Register::itemChanged);

  itemChangeEnabled = false;

  // Preload all diagnoses from file
  diagnoses = dx.getAllDiagnoses();
}

Register::~Register() {
  itemChangeEnabled = false;
  delete ui;
}

void Register::setData(const QList<HMISRow> &tableData) {
  this->data = tableData;
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

void Register::onSearchTextChanged(const QString &text) {
  if (text.trimmed().isEmpty()) {
    this->filteredData = data;
    populateTableWithData();
    return;
  }

  this->filteredData = {};

  // filter by patient ip number
  if (ui->comboBoxSearch->currentIndex() == 0) {
    for (HMISRow &row : data) {
      if (row.ipNumber.startsWith(text)) {
        filteredData << row;
      }
    }
  } else {
    // filter by diagnosis
    for (HMISRow &row : data) {
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
  ui->tableWidget->setColumnCount((int)headers.size());

  QHeaderView *horizontalHeader = ui->tableWidget->horizontalHeader();
  horizontalHeader->setSectionResizeMode(0, QHeaderView::Fixed);
  horizontalHeader->setSectionResizeMode(1, QHeaderView::Fixed);
  horizontalHeader->setSectionResizeMode(2, QHeaderView::Fixed);
  horizontalHeader->setSectionResizeMode(3, QHeaderView::Fixed);
  horizontalHeader->setSectionResizeMode(4, QHeaderView::Fixed);
  horizontalHeader->setSectionResizeMode(5, QHeaderView::Stretch);

  horizontalHeader->setStyleSheet(
      "font-family: Arial; font-size: 12px; background-color: lightgray;");

  ui->tableWidget->setStyleSheet("QHeaderView::section {font-size:12px; } "
                                 "QHeaderView::section:nth-of-type(odd) { "
                                 "background-color: beige; color:purple; }");

  // Allow editing via double click.
  ui->tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked);
  ui->tableWidget->setAlternatingRowColors(true);
  ui->tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  ui->tableWidget->setHorizontalHeaderLabels(headers);
}

void Register::itemChanged(QTableWidgetItem *item) {
  if (!itemChangeEnabled) {
    return;
  }

  auto showError = [&](const QString &msg) {
    QMessageBox::warning(this, "Validation Error", msg);
  };

  if (item) {
    // Get the row of the changed item
    int row = item->row();
    int column = item->column();

    // Ignore ID column
    if (column == 0) {
      return;
    }

    int columnCount = ui->tableWidget->columnCount();

    QStringList rowData;

    for (int col = 0; col < columnCount; ++col) {
      QTableWidgetItem *cellItem = ui->tableWidget->item(row, col);
      if (cellItem) {
        rowData.append(cellItem->text());
      }
    }

    // validate the rowData.
    //  headers = {"ID", "Patient ID", "Age Category", "Sex", "New Attendance",
    //  "Diagnosis"};
    for (auto &str : rowData) {
      if (str.isEmpty()) {
        return;
      }
    }

    // Validate sex
    if (rowData[3] != "Male" && rowData[3] != "Female") {
      showError("Invalid sex. Must be Male or Female");
      return;
    }

    // validate new attendance
    if (rowData[4] != "YES" && rowData[4] != "NO") {
      showError("Invalid attendance status. Must be YES or NO");
      return;
    }

    // Validate age category
    static const QStringList ageCategories = {
        "0 - 28 days",   "29 days - 4 years",  "5 - 9 years",
        "10 - 19 years", "20 years and above",
    };

    if (!ageCategories.contains(rowData[2])) {
      showError("Invalid age Category. Must be one of " +
                ageCategories.join(", "));
      return;
    }

    // Add diagnoses to file if they don't exit
    const HMISRow &hmisRow = {
        .id = rowData[0].toInt(),
        .ageCategory = rowData[2],
        .sex = rowData[3],
        .newAttendance = rowData[4],
        .diagnoses = rowData[5].split(","),
        .ipNumber = rowData[1],
    };

    QStringList missing;
    for (auto &diag : hmisRow.diagnoses) {
      if (!diagnoses.contains(diag)) {
        missing << diag;
        diagnoses << diag; // append to current diagnoses in memory
      }
    }

    // write diff to file.
    if (missing.size() > 0) {
      dx.appendDiagnoses(missing);
      qDebug() << "Saved " + QString::number(missing.size()) +
                      " new diagnoses to file"
               << "\n";
    }

    if (m_db->updateHMISRow(hmisRow.id, hmisRow)) {
      QMessageBox::information(this, "Success", "Row updated successfully");
    } else {
      showError("Error updating row");
    }
  }
}

void Register::populateTableWithData() {
  itemChangeEnabled =
      false; // disable itemChanged signal during populating of table.

  ui->tableWidget->clearContents();
  ui->tableWidget->setRowCount((int)filteredData.size());

  int row = 0;
  for (HMISRow &dataRow : filteredData) {
    ui->tableWidget->setItem(row, 0,
                             new QTableWidgetItem(QString::number(dataRow.id)));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(dataRow.ipNumber));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(dataRow.ageCategory));
    ui->tableWidget->setItem(row, 3, new QTableWidgetItem(dataRow.sex));
    ui->tableWidget->setItem(row, 4,
                             new QTableWidgetItem(dataRow.newAttendance));
    ui->tableWidget->setItem(
        row, 5, new QTableWidgetItem(dataRow.diagnoses.join(", ")));
    row++;
  }

  ui->tableWidget->horizontalHeader()->resizeSections(
      QHeaderView::ResizeToContents);
  ui->tableWidget->resizeColumnsToContents();
  itemChangeEnabled = true; // re-enable signal.
}
