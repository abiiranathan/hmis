#ifndef DIAGNOSIS_H
#define DIAGNOSIS_H
#include <QList>
#include <QString>

class Diagnosis {
public:
  Diagnosis();
  QList<QString> getAllDiagnoses();
  void appendDiagnoses(QStringList &missing);
};

#endif // DIAGNOSIS_H
