#ifndef REGISTER_H
#define REGISTER_H

#include <QAbstractItemModel>
#include <QMainWindow>
#include <QTableWidgetItem>

#include "HMISRow.hpp"
#include "database.hpp"

using Sex        = QString;
const Sex MALE   = QString("Male");
const Sex FEMALE = QString("Female");

// Age categories
const QString ZERO_TO_TWENTY_EIGHT_DAYS      = "0 - 28 days";
const QString TWENTY_NINE_DAYS_TO_FOUR_YEARS = "29 days - 4 years";
const QString FIVE_TO_NINE_YEARS             = "5 - 9 years";
const QString TEN_TO_NINETEEN_YEARS          = "10 - 19 years";
const QString TWENTY_YEARS_AND_ABOVE         = "20 years and above";

// Attendance statuses
const QString YES = "YES";
const QString NO  = "NO";

// QMap storing counts for different sexes
using SexMap = QMap<Sex, int>;

using AgeCategoryMap = QMap<QString, SexMap>;

// diagnoses: map[diagnosis][category][sex]
// attendances: map[attendance_status][category][sex]
using StatsMap = QMap<QString, QMap<QString, SexMap>>;

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
    void plotData(const StatsMap& dxMap, const StatsMap& attendanceMap);

private slots:
    void onSearchTextChanged(const QString& text);
    void onSearchTypeChanged(int index);
    void itemChanged(QTableWidgetItem* item);
    void deleteSelectedRow();
    void hideIDColumn();
    void disableIDColumn();
};

#endif  // REGISTER_H
