#ifndef REGISTER_H
#define REGISTER_H

#include <QAbstractItemModel>
#include <QDialog>
#include <QTableWidgetItem>

#include "HMISRow.h"
#include "database.h"
#include "diagnosis.h"

namespace Ui {
class Register;
}

class Register : public QDialog {
  Q_OBJECT

public:
  explicit Register(Database *db, QWidget *parent = nullptr);
  ~Register();

  void setData(QList<HMISRow> const &data);

private slots:
  void onSearchTextChanged(const QString &text);
  void onSearchTypeChanged(int index);
  void itemChanged(QTableWidgetItem *item);

private:
  Ui::Register *ui;
  Database *m_db;
  Diagnosis dx{};
  QList<QString> diagnoses{};
  bool itemChangeEnabled;

  QStringList headers;
  QList<HMISRow> data;
  QList<HMISRow> filteredData;

  void buildTableWidget();
  void populateTableWithData();
};

#endif // REGISTER_H
