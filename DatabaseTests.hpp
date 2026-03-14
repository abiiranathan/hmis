#ifndef HMIS_TESTS_H
#define HMIS_TESTS_H

/*  Tests — build with Qt Test:
 *
 *  Add to CMakeLists.txt:
 *    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)
 *    add_executable(HMISTests tests/DatabaseTests.hpp tests/main_test.cpp)
 *    target_link_libraries(HMISTests Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Sql)
 *
 *  Run: ./HMISTests
 */

#include <QtTest/QtTest>
#include "../database.hpp"
#include "../databaseOptions.hpp"

class DatabaseTests : public QObject {
    Q_OBJECT

    Database db;

    void connectInMemory() {
        db.Connect(ConnOptions(SqliteOptions(":memory:")));
        db.createSchema();
    }

private slots:

    // ── Schema ────────────────────────────────────────────────────
    void test_createSchema_opensSuccessfully() {
        connectInMemory();
        QVERIFY(db.isOpen());
    }

    // ── Default admin user ────────────────────────────────────────
    void test_defaultAdminExists() {
        connectInMemory();
        QVERIFY(db.userExists("admin"));
    }

    void test_defaultAdminAuthenticates() {
        connectInMemory();
        int uid = db.authenticateUser("admin", "admin123");
        QVERIFY(uid > 0);
        QCOMPARE(db.userRole(uid), QString("admin"));
    }

    void test_wrongPasswordRejected() {
        connectInMemory();
        QCOMPARE(db.authenticateUser("admin", "wrongpassword"), -1);
    }

    // ── Diagnoses ─────────────────────────────────────────────────
    void test_insertAndFetchDiagnosis() {
        connectInMemory();
        QVERIFY(db.insertDiagnoses({"Malaria Confirmed"}));

        auto all = db.getAllDiagnoses();
        QVERIFY(all.has_value());
        QCOMPARE(all->size(), 1);
        QCOMPARE(all->first().name, QString("Malaria Confirmed"));
    }

    void test_diagnosisExists_returnsTrue() {
        connectInMemory();
        db.insertDiagnoses({"Pneumonia"});
        QVERIFY(db.diagnosisExists("Pneumonia"));
    }

    void test_diagnosisExists_returnsFalse() {
        connectInMemory();
        QVERIFY(!db.diagnosisExists("NonExistentDisease"));
    }

    void test_insertDuplicate_fails() {
        connectInMemory();
        QVERIFY(db.insertDiagnoses({"Malaria"}));
        // Second insert of same name must fail (UNIQUE constraint)
        QVERIFY(!db.insertDiagnoses({"Malaria"}));
    }

    // ── HMIS rows ─────────────────────────────────────────────────
    void test_saveAndFetchRow() {
        connectInMemory();
        db.insertDiagnoses({"Hypertension"});

        NewHMISData data;
        data.ageCategory   = "20 years and above";
        data.sex           = "Female";
        data.newAttendance = "YES";
        data.diagnoses     = {"Hypertension"};
        data.ipNumber      = "001";
        data.month         = 3;
        data.year          = 2025;

        QVERIFY(db.saveNewRow(data));

        HMISData rows = db.fetchHMISData(2025, 3);
        QCOMPARE(rows.size(), 1);
        QCOMPARE(rows[0].ipNumber,      QString("001"));
        QCOMPARE(rows[0].sex,           QString("Female"));
        QCOMPARE(rows[0].ageCategory,   QString("20 years and above"));
        QCOMPARE(rows[0].newAttendance, QString("YES"));
        QVERIFY(rows[0].diagnoses.contains("Hypertension"));
    }

    void test_duplicateIPNumber_rejected() {
        connectInMemory();

        NewHMISData data;
        data.ageCategory   = "20 years and above";
        data.sex           = "Male";
        data.newAttendance = "YES";
        data.diagnoses     = {};
        data.ipNumber      = "042";
        data.month         = 1;
        data.year          = 2025;

        QVERIFY(db.saveNewRow(data));
        // Same IP + same month + same year must be rejected
        QVERIFY(!db.saveNewRow(data));
    }

    void test_updateRow() {
        connectInMemory();

        NewHMISData data;
        data.ipNumber = "005"; data.month = 4; data.year = 2025;
        data.ageCategory = "5 - 9 years"; data.sex = "Male";
        data.newAttendance = "YES"; data.diagnoses = {};
        QVERIFY(db.saveNewRow(data));

        HMISRow row = db.fetchHMISData(2025, 4).first();
        row.sex = "Female";
        QVERIFY(db.updateHMISRow(row));

        HMISRow updated = db.fetchHMISData(2025, 4).first();
        QCOMPARE(updated.sex, QString("Female"));
    }

    void test_deleteRow() {
        connectInMemory();

        NewHMISData data;
        data.ipNumber = "010"; data.month = 5; data.year = 2025;
        data.ageCategory = "29 days - 4 years"; data.sex = "Female";
        data.newAttendance = "NO"; data.diagnoses = {};
        QVERIFY(db.saveNewRow(data));

        HMISRow row = db.fetchHMISData(2025, 5).first();
        QVERIFY(db.deleteHMISRow(row.id));
        QCOMPARE(db.fetchHMISData(2025, 5).size(), 0);
    }

    // ── nextIPNumber ──────────────────────────────────────────────
    void test_nextIPNumber_incrementsCorrectly() {
        connectInMemory();

        NewHMISData data;
        data.ipNumber = "007"; data.month = 6; data.year = 2025;
        data.ageCategory = "20 years and above"; data.sex = "Male";
        data.newAttendance = "YES"; data.diagnoses = {};
        db.saveNewRow(data);

        QCOMPARE(db.nextIPNumber(2025, 6), QString("008"));
    }

    void test_nextIPNumber_emptyMonth() {
        connectInMemory();
        // Nothing saved for this month → returns empty string
        QCOMPARE(db.nextIPNumber(2025, 7), QString());
    }

    // ── Audit log ─────────────────────────────────────────────────
    void test_auditLogRecordsInsert() {
        connectInMemory();

        NewHMISData data;
        data.ipNumber = "099"; data.month = 8; data.year = 2025;
        data.ageCategory = "10 - 19 years"; data.sex = "Female";
        data.newAttendance = "YES"; data.diagnoses = {};
        db.saveNewRow(data, "testuser");

        auto log = db.getAuditLog(10);
        QVERIFY(!log.isEmpty());
        bool found = false;
        for (const AuditEntry& e : log)
            if (e.username == "testuser" && e.action == "INSERT") { found = true; break; }
        QVERIFY(found);
    }

    // ── CSV export ────────────────────────────────────────────────
    void test_exportCSV_createsFile() {
        connectInMemory();

        NewHMISData data;
        data.ipNumber = "001"; data.month = 1; data.year = 2026;
        data.ageCategory = "20 years and above"; data.sex = "Male";
        data.newAttendance = "YES"; data.diagnoses = {"Malaria Confirmed"};
        db.insertDiagnoses({"Malaria Confirmed"});
        db.saveNewRow(data);

        QString path = QDir::tempPath() + "/hmis_test_export.csv";
        QString err = db.exportToCSV(2026, 1, path);
        QVERIFY2(err.isEmpty(), qPrintable(err));
        QVERIFY(QFile::exists(path));
        QFile::remove(path);
    }

    void test_exportCSV_noData_returnsError() {
        connectInMemory();
        QString err = db.exportToCSV(1999, 1, "/tmp/empty.csv");
        QVERIFY(!err.isEmpty());
    }

    // ── User management ───────────────────────────────────────────
    void test_createAndAuthenticateUser() {
        connectInMemory();
        QVERIFY(db.createUser("nurserose", "securepass", "clerk"));
        int uid = db.authenticateUser("nurserose", "securepass");
        QVERIFY(uid > 0);
        QCOMPARE(db.userRole(uid), QString("clerk"));
    }

    void test_userExists_caseSensitivity() {
        connectInMemory();
        db.createUser("DoctorAlex", "pass", "admin");
        // username stored as lowercase
        QVERIFY(db.userExists("doctораlex") || db.userExists("doctoralex"));
    }

    // ── MonthlyStats ──────────────────────────────────────────────
    void test_monthlyStats_incrementAndQuery() {
        MonthlyStats stats;
        stats.increment("Malaria", "5 - 9 years", "Male");
        stats.increment("Malaria", "5 - 9 years", "Male");
        stats.increment("Malaria", "5 - 9 years", "Female");

        QCOMPARE(stats.count("Malaria", "5 - 9 years", "Male"),   2);
        QCOMPARE(stats.count("Malaria", "5 - 9 years", "Female"), 1);
        QCOMPARE(stats.total("Malaria"), 3);
        QVERIFY(stats.hasAnyNonZero("Malaria"));
        QVERIFY(!stats.hasAnyNonZero("Pneumonia"));
    }
};

#endif // HMIS_TESTS_H
