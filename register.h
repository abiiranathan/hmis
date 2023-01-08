#ifndef REGISTER_H
#define REGISTER_H

#include <QDialog>

#include "HMISRow.h"

namespace Ui {
class Register;
}

class Register : public QDialog {
  Q_OBJECT

 public:
  explicit Register(QWidget *parent = nullptr);
  ~Register();

  void setData(QList<HMISRow> const &data);

 private slots:
  void on_search_textChanged(const QString &arg1);

 private:
  Ui::Register *ui;

  QStringList headers;
  QList<HMISRow> data;
  QList<HMISRow> filteredData;

  void buildTableWidget();
  void populateTableWithData();
};

#endif  // REGISTER_H
