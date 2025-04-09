#ifndef HMISROW_H
#define HMISROW_H

#include <QList>
#include <QString>

class HMISRow {
public:
  int id;                // ID of the record
  QString ageCategory;   // HMIS age category
  QString sex;           // Sex (Male | Female)
  QString newAttendance; // YES | NO
  QStringList diagnoses; // List of diagnoses
  QString ipNumber;      // IP Number
  int year;              // Year
  int month;             // Month

  QString toString();
};

#endif // HMISROW_H
