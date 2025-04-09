#include "diagnosis.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTextStream>

Diagnosis::Diagnosis() {}

QList<QString> Diagnosis::getAllDiagnoses() {
  QList<QString> diagnoses;

  // Get the executable's directory.
  QString appDirPath = QCoreApplication::applicationDirPath();

  // Construct the full file path.
  QString filePath = appDirPath + "/diagnoses.txt";

  // Open the file for reading.
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    // Handle the error
    qDebug() << "Error reading file:" << file.errorString();
    QMessageBox::critical(nullptr, "Error loading diagnoses",
                          file.errorString());
    return diagnoses;
  }

  // Create a text stream to read the file.
  QTextStream in(&file);

  while (!in.atEnd()) {
    QString line = in.readLine();
    if (line.trimmed().isEmpty()) {
      continue;
    }
    diagnoses.append(line.trimmed());
  }

  // Close the file.
  file.close();
  return diagnoses;
}

void Diagnosis::appendDiagnoses(QStringList &missing) {
  // Get the executable's directory.
  QString appDirPath = QCoreApplication::applicationDirPath();

  // Construct the full file path.
  QString filePath = appDirPath + "/diagnoses.txt";

  // Open the file for appending.
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
    // Handle the error
    qDebug() << "Error opening file:" << file.errorString();
    QMessageBox::critical(nullptr, "Error opening file", file.errorString());
    return;
  }

  // Create a QTextStream to write to the file
  QTextStream out(&file);

  // Append each diagnosis in the missing list to the file
  for (const QString &diagnosis : missing) {
    out << diagnosis << "\n"; // Write each diagnosis followed by a newline
  }

  // Close the file after writing
  file.close();
}
