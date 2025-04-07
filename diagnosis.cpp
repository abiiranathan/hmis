#include "diagnosis.h"

#include <QFile>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTextStream>

Diagnosis::Diagnosis() {}

QList<QString> Diagnosis::getAllDiagnoses() {
  QList<QString> diagnoses;

  // Open the file
  QFile file(":/diagnoses.txt");
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    // Handle the error
    QMessageBox::critical(nullptr, "Error loading diagnoses",
                          file.errorString());
    return diagnoses;
  }

  // Create a text stream to read the file
  QTextStream in(&file);

  while (!in.atEnd()) {
    QString line = in.readLine();
    if (line.trimmed().isEmpty()) {
      continue;
    }

    diagnoses.append(line.trimmed());
  }

  // Close the file
  file.close();
  return diagnoses;
}
