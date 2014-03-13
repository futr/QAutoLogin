#include "loginwebauth.h"

LoginWebAuth::LoginWebAuth(QObject *parent) :
    QObject(parent),
    m_loginCgiName( "Login.cgi" ),
    m_logoutCgiName( "Logout.cgi" ),
    m_cgiDir( "cgi-bin" ),
    m_manager( new QNetworkAccessManager( this ) ),
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
    QNetworkRequest request;
    QString send_data;

    send_data = "uid=" + userName() + "&pwd=" + password();

    // URLを設定
    request.setUrl( QUrl( serverUrl() + m_cgiDir + "/" + m_loginCgiName ) );
    request.setRawHeader( "Content-type", "application/x-www-form-urlencoded" );

    // POST
    QNetworkReply *reply = m_manager->post( request, send_data.toUtf8() );

    // SSLエラー無視
    if ( ignoreSslError() ) {
        reply->ignoreSslErrors();
    }

    // シグナル接続
    connect( reply, SIGNAL(finished()), this, SLOT(replyFinished()) );
    connect( reply, SIGNAL(sslErrors(QList<QSslError>)), this, SIGNAL(sslErrorOccurred(QList<QSslError>)) );
    connect( reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SIGNAL(networkErrorOccurred(QNetworkReply::NetworkError)) );
}

void LoginWebAuth::logout()
{
    // ログアウト
    QString send_data;

    // AccessManager mode
    QNetworkRequest request;

    // URL設定
    request.setUrl( QUrl( serverUrl() + m_cgiDir + "/" + m_logoutCgiName ) );
    request.setRawHeader( "Content-type", "application/x-www-form-urlencoded" );

    // POST
    QNetworkReply *reply = m_manager->post( request, send_data.toUtf8() );

    // 必要ならSSLエラーを無視
    reply->ignoreSslErrors();

    // connect
    connect( reply, SIGNAL(finished()), this, SLOT(replyFinished()) );
    connect( reply, SIGNAL(sslErrors(QList<QSslError>)), this, SIGNAL(sslErrorOccurred(QList<QSslError>)) );
    connect( reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SIGNAL(networkErrorOccurred(QNetworkReply::NetworkError)) );
}

void LoginWebAuth::setServerUrl( QString url )
{
    m_serverUrl = url;

    emit serverUrlChanged();
}

void LoginWebAuth::setUserName( QString name )
{
    m_userName = name;

    emit userNameChanged();
}

void LoginWebAuth::setPassword( QString password )
{
    m_password = password;

    emit passwordChanged();
}

void LoginWebAuth::setIgnoreSslError( bool enable )
{
    m_ignoreSslError = enable;

    emit ignoreSslErrorChanged();
}

void LoginWebAuth::replyFinished()
{
    // 通信完了
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );

    // バッファに読み込み
    QByteArray buffer( reply->readAll() );

    // 受信データー解析
    QXmlStreamReader reader( buffer );
    QString loginTime;
    QString logoutTime;
    AuthStatus stat = None;

    while ( !reader.atEnd() ) {
        // 次の要素を読む
        reader.readNext();

        // タグ開始
        if ( reader.isStartElement() ) {
            if ( reader.name() == "span" && reader.attributes().value( "id" ).toString() == "loginTime" ) {
                // ログイン時間取得
                loginTime = reader.readElementText();

                // ログイン時間設定
                m_loginTime = QDateTime::fromString( loginTime, Qt::ISODate );
                emit loginTimeChanged();

                // ログイン成功
                stat = LoginSucceeded;
            } else if ( reader.name() == "span" && reader.attributes().value( "id" ).toString() == "logoutTime" ) {
                // ログアウト時間取得
                logoutTime = reader.readElementText();

                // ログアウト時間設定
                m_autoLogoutTime = QDateTime::fromString( logoutTime, Qt::ISODate );
                emit autoLogoutTimeChanged();
            } else if ( reader.name() == "p" && reader.attributes().value( "class" ).toString() == "subcaption" ) {
                // ログインかログアウトか判別
                QString mode = reader.readElementText();

                if ( mode == "Logout" || mode == "ログアウト" ) {
                    stat = LogoutSucceeded;
                }
            }
        }
    }

    // 失敗を判別
    if ( stat == None ) {
        // ログインかログアウトか判定
        if ( reply->url().toString().contains( m_loginCgiName ) ) {
            stat = LoginFailed;
        } else {
            stat = LogoutFailed;
        }
    }

    // シグナル送信
    switch ( stat ) {
    case LoginSucceeded:
        emit succeeded();
        emit loginSucceeded();
        break;
    case LogoutSucceeded:
        emit succeeded();
        emit logoutSucceeded();
        break;
    case LoginFailed:
        emit failed();
        emit loginFailed();
        break;
    case LogoutFailed:
        emit failed();
        emit logoutFailed();
        break;
    default:
        emit failed();
    }

    emit finished( stat );

    // リプライを解放
    reply->deleteLater();
}
