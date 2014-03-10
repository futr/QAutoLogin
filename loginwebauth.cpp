#include "loginwebauth.h"

LoginWebAuth::LoginWebAuth(QObject *parent) :
    QObject(parent),
    m_ignoreSslError( false )
{
}

QString LoginWebAuth::userName() const
{
    return m_userName;
}

QString LoginWebAuth::password() const
{
    return m_password;
}

QString LoginWebAuth::serverUrl() const
{
    return m_serverUrl;
}

bool LoginWebAuth::ignoreSslError() const
{
    return m_ignoreSslError;
}

QDateTime LoginWebAuth::loginTime() const
{
    return m_loginTime;
}

QDateTime LoginWebAuth::autoLogoutTime() const
{
    return m_autoLogoutTime;
}

void LoginWebAuth::login()
{
    // ログイン
}

void LoginWebAuth::logout()
{
    // ログアウト
}

void LoginWebAuth::setServerUrl( QString url )
{
    m_serverUrl = url;
}

void LoginWebAuth::setUserName( QString name )
{
    m_userName = name;
}

void LoginWebAuth::setPassword( QString password )
{
    m_password = password;
}

void LoginWebAuth::setIgnoreSslError( bool enable )
{
    m_ignoreSslError = enable;
}
