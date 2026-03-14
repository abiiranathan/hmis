#ifndef REGISTER_H
#define REGISTER_H

#include <QAbstractItemModel>
#include <QMainWindow>
#include <QTableWidgetItem>

#include "HMISRow.hpp"
#include "MonthlyStats.hpp"
#include "database.hpp"

namespace Ui { class Register; }

class Register : public QMainWindow {
    Q_OBJECT

    Ui::Register* ui;
    Database* m_db;
    User m_currentUser;
    bool itemChangeEnabled = false;

    QStringList headers;
    QList<HMISRow> data;
    QList<HMISRow> filteredData;
    int year, month;

    void buildTableWidget();
    void populateTableWithData();
    void hideIDColumn();

public:
    explicit Register(Database* db, int year, int month, QWidget* parent = nullptr);
    ~Register() override;

    void setCurrentUser(const User& user) { m_currentUser = user; }
    void setData(const QList<HMISRow>& data);
    void plotData(const MonthlyStats& dxMap, const MonthlyStats& attendanceMap);

private slots:
    void onSearchTextChanged(const QString& text);
    void onSearchTypeChanged(int index);
    void itemChanged(QTableWidgetItem* item);
    void deleteSelectedRow();
};

#endif  // REGISTER_H
