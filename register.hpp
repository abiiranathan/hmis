#ifndef REGISTER_H
#define REGISTER_H

#include <QAbstractItemModel>
#include <QMainWindow>
#include <QTableWidgetItem>

#include "HMISRow.hpp"
#include "database.hpp"

namespace Ui {
class Register;
}

class Register : public QMainWindow {
    Q_OBJECT

    Ui::Register* ui;

    Database* m_db;
    bool itemChangeEnabled;

    QStringList headers;
    QList<HMISRow> data;
    QList<HMISRow> filteredData;
    int year, month;

    void buildTableWidget();
    void populateTableWithData();

public:
    explicit Register(Database* db, int year, int month, QWidget* parent = nullptr);
    ~Register() override;

    void setData(QList<HMISRow> const& data);

private slots:
    void onSearchTextChanged(const QString& text);
    void onSearchTypeChanged(int index);
    void itemChanged(QTableWidgetItem* item);
    void deleteSelectedRow();
    void hideIDColumn();
    void disableIDColumn();
};

#endif  // REGISTER_H
