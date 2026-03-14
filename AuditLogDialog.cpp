#include "AuditLogDialog.hpp"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

AuditLogDialog::AuditLogDialog(Database& db, QWidget* parent) : QDialog(parent), m_db(db) {
    setWindowTitle("Audit Log");
    setMinimumSize(900, 500);

    auto* vLayout = new QVBoxLayout(this);

    auto* titleLabel = new QLabel("System Audit Log", this);
    titleLabel->setStyleSheet("font-size:16px; font-weight:bold; margin-bottom:6px;");
    vLayout->addWidget(titleLabel);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({"#", "User", "Action", "Table", "Record ID", "Detail", "Timestamp"});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    vLayout->addWidget(m_table);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* closeBtn = new QPushButton("Close", this);
    closeBtn->setFixedWidth(100);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    vLayout->addLayout(btnLayout);

    loadData();
}

void AuditLogDialog::loadData() {
    auto entries = m_db.getAuditLog(1000);
    m_table->setRowCount(static_cast<int>(entries.size()));

    int row = 0;
    for (const AuditEntry& e : entries) {
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(e.id)));
        m_table->setItem(row, 1, new QTableWidgetItem(e.username));
        m_table->setItem(row, 2, new QTableWidgetItem(e.action));
        m_table->setItem(row, 3, new QTableWidgetItem(e.tableName));
        m_table->setItem(row, 4, new QTableWidgetItem(QString::number(e.recordId)));
        m_table->setItem(row, 5, new QTableWidgetItem(e.detail));
        m_table->setItem(row, 6, new QTableWidgetItem(e.changedAt));

        QColor colour;
        if (e.action == "INSERT") {
            colour = QColor("#e6f4ea");
        } else if (e.action == "UPDATE") {
            colour = QColor("#fff8e1");
        } else if (e.action == "DELETE") {
            colour = QColor("#fce8e6");
        }

        if (colour.isValid()) {
            for (int col = 0; col < 7; ++col) {
                if (m_table->item(row, col) != nullptr) {
                    m_table->item(row, col)->setBackground(colour);
                }
            }
        }
        row++;
    }
    m_table->resizeColumnsToContents();
}
