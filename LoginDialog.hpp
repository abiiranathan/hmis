#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <optional>
#include "database.hpp"

class LoginDialog : public QDialog {
    Q_OBJECT
  public:
    explicit LoginDialog(Database& db, QWidget* parent = nullptr);

    // Returns the authenticated user after exec() == Accepted
    [[nodiscard]] std::optional<User> authenticatedUser() const { return m_user; }

  private slots:
    void attemptLogin();

  private:
    Database& m_db;
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QLabel* m_errorLabel;
    std::optional<User> m_user;

    int m_attempts = 0;
    static constexpr int MAX_ATTEMPTS = 5;
};

#endif  // LOGINDIALOG_H
