#ifndef LOGINWEBAUTH_H
#define LOGINWEBAUTH_H

#include <QObject>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QSslError>
#include <QNetworkReply>

class LoginWebAuth : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)
    Q_PROPERTY(QString userName READ userName WRITE setUserName NOTIFY userNameChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(bool ignoreSslError READ ignoreSslError WRITE setIgnoreSslError NOTIFY ignoreSslErrorChanged)
    Q_PROPERTY(QDateTime loginTime READ loginTime NOTIFY loginTimeChanged)
    Q_PROPERTY(QDateTime autoLogoutTime READ autoLogoutTime NOTIFY autoLogoutTimeChanged)
public:
    explicit LoginWebAuth(QObject *parent = 0);

    QString userName() const;
    QString password() const;
    QString serverUrl() const;
    bool ignoreSslError() const;
    QDateTime loginTime() const;
    QDateTime autoLogoutTime() const;

signals:
    void serverUrlChanged();
    void userNameChanged();
    void passwordChanged();
    void ignoreSslErrorChanged();
    void loginTimeChanged();
    void autoLogoutTimeChanged();

    void succeeded();
    void failed();
    void loginSucceeded();
    void loginFailed();
    void logoutSucceeded();
    void logoutFailed();
    void sslErrorOccurred();
    void errorOccurred();

public slots:
    void login();
    void logout();

    void setServerUrl( QString url );
    void setUserName( QString name );
    void setPassword( QString password );
    void setIgnoreSslError( bool enable = true );

private slots:
    //void sslError( const QList<QSslError> & errors );
    //void networkError( QNetworkReply::NetworkError code );

private:
    QString m_userName;
    QString m_password;
    QString m_serverUrl;
    QDateTime m_loginTime;
    QDateTime m_autoLogoutTime;
    bool m_ignoreSslError;
};

#endif // LOGINWEBAUTH_H
