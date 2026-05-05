#ifndef AUDITLOGDIALOG_H
#define AUDITLOGDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include "database.hpp"

class AuditLogDialog : public QDialog {
    Q_OBJECT
  public:
    explicit AuditLogDialog(Database& db, QWidget* parent = nullptr);

  private:
    void loadData();
    Database& m_db;
    QTableWidget* m_table;
};

#endif  // AUDITLOGDIALOG_H
