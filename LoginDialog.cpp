#include "LoginDialog.hpp"
#include <QApplication>
#include <QFrame>
#include <QGridLayout>
#include <QIcon>

LoginDialog::LoginDialog(Database& db, QWidget* parent) : QDialog(parent), m_db(db) {
    setWindowTitle("HMIS 105 — Login");
    setFixedSize(360, 260);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 32, 32, 24);
    layout->setSpacing(12);

    // Title
    auto* title = new QLabel("HMIS 105", this);
    title->setStyleSheet("font-size:22px; font-weight:bold; color:#005BAC;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto* subtitle = new QLabel("Health Management Information System", this);
    subtitle->setStyleSheet("font-size:11px; color:gray;");
    subtitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitle);

    layout->addSpacing(8);

    // Username
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("Username");
    m_usernameEdit->setStyleSheet(
        "QLineEdit { border:1px solid #ccc; border-radius:5px; padding:8px; font-size:13px; }"
        "QLineEdit:focus { border-color:#0073D4; }");
    m_usernameEdit->setFixedHeight(36);
    layout->addWidget(m_usernameEdit);

    // Password
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("Password");
    m_passwordEdit->setStyleSheet(
        "QLineEdit { border:1px solid #ccc; border-radius:5px; padding:8px; font-size:13px; }"
        "QLineEdit:focus { border-color:#0073D4; }");
    m_passwordEdit->setFixedHeight(36);
    layout->addWidget(m_passwordEdit);

    // Error label (hidden initially)
    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet("color:red; font-size:11px;");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->hide();
    layout->addWidget(m_errorLabel);

    // Login button
    auto* loginBtn = new QPushButton("Sign In", this);
    loginBtn->setFixedHeight(38);
    loginBtn->setStyleSheet(
        "QPushButton { background:#005BAC; color:white; border-radius:5px; font-size:13px; }"
        "QPushButton:hover { background:#0073D4; }"
        "QPushButton:pressed { background:#004080; }");
    layout->addWidget(loginBtn);

    connect(loginBtn, &QPushButton::clicked, this, &LoginDialog::attemptLogin);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::attemptLogin);
    connect(m_usernameEdit, &QLineEdit::returnPressed, this, [this] { m_passwordEdit->setFocus(); });
}

void LoginDialog::attemptLogin() {
    if (m_attempts >= MAX_ATTEMPTS) {
        m_errorLabel->setText("Too many failed attempts. Restart the application.");
        m_errorLabel->show();
        return;
    }

    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        m_errorLabel->setText("Please enter username and password.");
        m_errorLabel->show();
        return;
    }

    auto user = m_db.authenticate(username, password);
    if (user) {
        m_user = user;
        accept();
    } else {
        m_attempts++;
        m_errorLabel->setText(QString("Invalid credentials. (%1/%2 attempts)").arg(m_attempts).arg(MAX_ATTEMPTS));
        m_errorLabel->show();
        m_passwordEdit->clear();
        m_passwordEdit->setFocus();
    }
}
