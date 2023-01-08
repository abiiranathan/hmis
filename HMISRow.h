#ifndef HMISROW_H
#define HMISROW_H

#include <QList>
#include <QString>

class HMISRow {
 public:
  int id;
  QString ageCategory;
  QString sex;
  QString newAttendance;
  QStringList diagnoses;
  QString ipNumber;
  int year;
  int month;

  QString toString();
};

#endif  // HMISROW_H
