#include "mainwidget.h"
#include "ui_mainwidget.h"

MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWidget),
    m_ipconfig( new QProcess( this ) ),
    m_login( false )
{
    ui->setupUi(this);

    // setup
    createAction();
    createTrayIcon();

    // set echomode
    ui->passEdit->setEchoMode( QLineEdit::Password );

    // setting
#ifdef Q_OS_MAC
    setting = new QSettings( QSettings::UserScope, "futr", "QAutoLogin" );
    setting->setFallbacksEnabled( false );
#else
    setting = new QSettings( "conf.ini", QSettings::IniFormat, this );
#endif

    // load setting
    loadConfigSlot();

    // 読み込まれた設定によってフォーカスを制御
    if ( setting->value( "Auth" ).toString().length() == 0 ) {
        ui->serverNameEdit->setFocus();
    } else if ( setting->value( "ID" ).toString().length() == 0 ) {
        ui->idEdit->setFocus();
    } else {
        ui->passEdit->setFocus();
    }

    // connect timer
    connect( &statusTimer, SIGNAL(timeout()), SLOT(updateStatusSlot()) );
    connect( &timer, SIGNAL(timeout()), this, SLOT(loginSlot()) );

    // ネットワークのエラーを接続
    connect( &webAuth, SIGNAL(sslErrorOccurred(QList<QSslError>)), this, SLOT(sslErrorSlot(QList<QSslError>)) );
    connect( &webAuth, SIGNAL(networkErrorOccurred(QNetworkReply::NetworkError)), this, SLOT(networkErrorSlot(QNetworkReply::NetworkError)) );
    connect( &webAuth, SIGNAL(finished(LoginWebAuth::AuthStatus)), this ,SLOT(readyReadSlot(LoginWebAuth::AuthStatus)) );
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::on_loginButton_clicked()
{
    // login button

    // clear status
    clearStatusSlot();

    // stop all timer
    timer.stop();
    statusTimer.stop();

    // disable stop button
    ui->stopTimerButton->setEnabled( false );

    if ( ui->repeatCheckBox->isChecked() ) {
        // repeat login
        int repeatTime;
        int randTime;

        // 必要なら乱数でぶらす
        repeatTime = ui->repatSpinBox->value();

        if ( ui->randomizeCheckBox->isChecked() && ui->randomizeSpinBox->value() != 0 ) {
            randTime = qrand() % ui->randomizeSpinBox->value();
            
            if ( qrand() % 2 ) {
                randTime *= -1;
            }
            
            repeatTime += randTime;
        }

        if ( repeatTime < 0 ) {
            repeatTime = 1;
        }

        timer.setInterval( repeatTime * 60 * 1000 );

        // set progress
        ui->progressBar->setMaximum( repeatTime );
        ui->progressBar->setValue( repeatTime );

        // set status timer
        statusTimer.setInterval( 1000 * 60 );

        statusTimer.start();

        // start timer
        timer.start();

        // button enable
        ui->stopTimerButton->setEnabled( true );
    }

    // login
    loginSlot();
}

void MainWidget::loginSlot()
{
    // ログイン
    webAuth.setServerUrl( ui->serverNameEdit->text() );
    webAuth.setUserName( ui->idEdit->text() );
    webAuth.setPassword( ui->passEdit->text() );

    // SSLエラー無視
    webAuth.setIgnoreSslError();

    webAuth.login();
}

void MainWidget::logoutSlot()
{
    // logout slot
    webAuth.setServerUrl( ui->serverNameEdit->text() );
    webAuth.setUserName( ui->idEdit->text() );
    webAuth.setPassword( ui->passEdit->text() );

    // SSLエラー無視
    webAuth.setIgnoreSslError();

    webAuth.logout();
}

void MainWidget::readyReadSlot( LoginWebAuth::AuthStatus stat )
{
    // 通信完了

    // メッセージ作成
    QString info;
    QString title;

    if ( stat == LoginWebAuth::LoginSucceeded ) {
        // ログイン成功
        title = tr( "Login" );
        info  = QString() + tr( "Login time\t : " ) + webAuth.loginTime().toString( Qt::DefaultLocaleShortDate )
                + tr( "\nLogout time\t : " ) + webAuth.autoLogoutTime().toString( Qt::DefaultLocaleShortDate );

        if ( trayIcon->isVisible() ) {
            trayIcon->showMessage( title, info );
        }
    } else if ( stat == LoginWebAuth::LogoutSucceeded ) {
        // ログアウト成功
        title = tr( "Logout" );
        info  = tr( "Logout succeeded" );

        if ( trayIcon->isVisible() ) {
            trayIcon->showMessage( title, info );
        }
    } else {
        // error
        title = tr( "Error" );
        info  = tr( "Some errors occurred" );

        if ( trayIcon->isVisible() ) {
            trayIcon->showMessage( title, info, QSystemTrayIcon::Warning );
        }
    }

    // ラベルに表示
    ui->messageLabel->setText( title + "\n" + info );

    // 成功していて初回ログインでかつ必要ならIP更新
    if ( ui->renewIPCheckBox->isChecked() && ( stat == LoginWebAuth::LoginSucceeded && m_login == false ) ) {
        renewIP();
    }

    // ログイン済み
    if ( stat == LoginWebAuth::LoginSucceeded ) {
        m_login = true;
    }

    // ログアウトでかつ必要ならIP更新
    if ( ui->renewIPLogoutCheckBox->isChecked() && stat == LoginWebAuth::LogoutSucceeded ) {
        renewIP();
    }
}

void MainWidget::saveConfigSlot()
{
    // save config

    // save check
    setting->setValue( "Repeat", ui->repeatCheckBox->isChecked() );
    setting->setValue( "AutoMinimize", ui->autoToTrayCheckBox->isChecked() );
    setting->setValue( "AutoStart", ui->autoStartCheckBox->isChecked() );
    setting->setValue( "ConfigId", ui->saveIdCheckBox->isChecked() );
    setting->setValue( "ConfigPass", ui->savePassCheckBox->isChecked() );
    setting->setValue( "RepeatMin", ui->repatSpinBox->value() );
    setting->setValue( "Auth", ui->serverNameEdit->text() );
    setting->setValue( "AutoLogout", ui->autoLogoutCheckBox->isChecked() );
    setting->setValue( "AutoRenewIP", ui->renewIPCheckBox->isChecked() );
    setting->setValue( "AutoRenewIPLogout", ui->renewIPLogoutCheckBox->isChecked() );
    setting->setValue( "RandomizeEnable", ui->randomizeCheckBox->isChecked() );
    setting->setValue( "RandomizeRange", ui->randomizeSpinBox->value() );

    // if need id,pass
    if ( ui->saveIdCheckBox->isChecked() ) {
        setting->setValue( "ID", ui->idEdit->text() );
    }

    if ( ui->savePassCheckBox->isChecked() ) {
        setting->setValue( "Pass", ui->passEdit->text() );
    }
}

void MainWidget::loadConfigSlot()
{
    // load config

    // load check
    ui->repeatCheckBox->setChecked( setting->value( "Repeat" ).toBool() );
    ui->autoToTrayCheckBox->setChecked( setting->value( "AutoMinimize" ).toBool() );
    ui->autoStartCheckBox->setChecked( setting->value( "AutoStart" ).toBool() );
    ui->saveIdCheckBox->setChecked( setting->value( "ConfigId" ).toBool() );
    ui->savePassCheckBox->setChecked( setting->value( "ConfigPass" ).toBool() );
    ui->repatSpinBox->setValue( setting->value( "RepeatMin" ).toInt() );
    ui->serverNameEdit->setText( setting->value( "Auth" ).toString() );
    ui->autoLogoutCheckBox->setChecked( setting->value( "AutoLogout" ).toBool() );
    ui->renewIPLogoutCheckBox->setChecked( setting->value( "AutoRenewIPLogout" ).toBool() );
    ui->renewIPCheckBox->setChecked( setting->value( "AutoRenewIP" ).toBool() );
    ui->randomizeCheckBox->setChecked( setting->value( "RandomizeEnable" ).toBool() );
    ui->randomizeSpinBox->setValue( setting->value( "RandomizeRange" ).toInt() );

    // if need id,pass
    if ( ui->saveIdCheckBox->isChecked() ) {
        ui->idEdit->setText( setting->value( "ID" ).toString() );
    }

    if ( ui->savePassCheckBox->isChecked() ) {
        ui->passEdit->setText( setting->value( "Pass" ).toString() );
    }

    // do setting
    if ( ui->autoStartCheckBox->isChecked() ) {
        // 起動時に自動ログイン

        // Windowsの場合いったんIPをrenewする
        renewIP();
        m_ipconfig->waitForFinished();

        on_loginButton_clicked();
    }

    if ( ui->autoToTrayCheckBox->isChecked() ) {
        // minimize to tray
        QTimer::singleShot( 100, this, SLOT(minimizeSlot()) );
    }
}

void MainWidget::minimizeSlot()
{
    // minimize to tray
    trayIcon->show();

    /*
    trayIcon->showMessage( tr( "Minimize to tray" ),
        tr( "This application is still running. To quit please click this icon and select Close" ) );
    */

    hide();
}

void MainWidget::updateStatusSlot()
{
    // update status

    // update progress bar
    if ( timer.isActive() ) {
        ui->progressBar->setValue( ui->progressBar->value() - 1 );
    }
}

void MainWidget::clearStatusSlot()
{
    // clearStatus
    ui->progressBar->setValue( 0 );
}

void MainWidget::sslErrorSlot(const QList<QSslError> &errors)
{
    // sslError Slot
    QNetworkReply *reply = qobject_cast< QNetworkReply *>( sender() );
    QString errString;

    // iOSでは不完全型となっているため直接アクセスしない
#ifndef Q_OS_IOS
    foreach ( QSslError err, errors ) {
        errString += err.errorString() + "\n";
    }
#endif

    // 全エラー無視（本来は確認すべき）
    reply->ignoreSslErrors();

    // view ssl error
    if ( isVisible() ) {
        QMessageBox::warning( this, tr( "Ssl warning" ), errString + tr( "\n\nThese \"SslError\" are ignored." ) );
    }
}

void MainWidget::trayIconClickSlot( QSystemTrayIcon::ActivationReason reason )
{
    // tray icon clicked
    if ( reason == QSystemTrayIcon::Trigger ) {
        // Macの場合ここでhideすると落ちるためなにもしない
#ifdef Q_OS_MAC
        return;
#endif

        trayIcon->hide();
        this->show();
    }
}

void MainWidget::networkErrorSlot( QNetworkReply::NetworkError code )
{
    // network error
    Q_UNUSED( code )

    if ( isVisible() ) {
        QMessageBox::warning( this, tr( "Network error" ), tr( "Some network error occurred" ) );
    }
}

void MainWidget::checkIpSlot()
{
    // check ip
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
             ui->ipEdit->setText( address.toString() );
    }
}

void MainWidget::renewIP()
{
    // IPを更新する ( Windows only )
#ifdef Q_OS_WIN
    // メッセージ表示
    ui->messageLabel->setText( tr( "Updating IP address" ) );
    QApplication::processEvents();

    // ブロックしてipconfigでIP更新
    m_ipconfig->start( "ipconfig", QStringList() << "/release" );
    m_ipconfig->waitForFinished();
    m_ipconfig->start( "ipconfig", QStringList() << "/renew" );
    m_ipconfig->waitForFinished();

    ui->messageLabel->setText( tr( "Update complete" ) );
#endif
}

void MainWidget::on_aboutButton_clicked()
{
    // aboutQt
    QMessageBox::aboutQt( this );
}

void MainWidget::on_closeButton_clicked()
{
    close();
}

void MainWidget::on_stopTimerButton_clicked()
{
    // stop timer
    timer.stop();
    statusTimer.stop();

    ui->stopTimerButton->setEnabled( false );
}

void MainWidget::createTrayIcon()
{
    // create tray icon

    // create menu
    trayMenu = new QMenu( this );

    trayMenu->addAction( showAction );
    trayMenu->addSeparator();
    trayMenu->addAction( connectAction );
    trayMenu->addAction( disconnectAction );
    trayMenu->addSeparator();
    trayMenu->addAction( closeAction );

    // setup icon
    trayIcon = new QSystemTrayIcon( this );

    trayIcon->setContextMenu( trayMenu );
    trayIcon->setIcon( QIcon( ":/image/icon.png" ) );
    trayIcon->setToolTip( tr( "QAutoLogin" ) );

    // connect
    connect( trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconClickSlot(QSystemTrayIcon::ActivationReason)) );
}

void MainWidget::createAction()
{
    // create actions
    closeAction = new QAction( tr( "&Close" ), this );
    connectAction = new QAction( tr( "&Login" ), this );
    disconnectAction = new QAction( tr( "Log&out" ), this );
    showAction = new QAction( tr( "&Show" ), this );

    connect( closeAction, SIGNAL(triggered()), this, SLOT(close()) );
    connect( connectAction, SIGNAL(triggered()), this, SLOT(on_loginButton_clicked()) );
    connect( disconnectAction, SIGNAL(triggered()), this, SLOT(on_logoutButton_clicked()) );
    connect( showAction, SIGNAL(triggered()), SLOT(show()) );
}

void MainWidget::closeEvent(QCloseEvent *)
{
    // close event
    if ( trayIcon->isVisible() ) {
        trayIcon->hide();
    }

    // 必要ならログアウト
    if ( ui->autoLogoutCheckBox->isChecked() ) {
        // ログアウト処理が完了するまでブロック
        QEventLoop loop;
        connect( &webAuth, SIGNAL(finished(LoginWebAuth::AuthStatus)), &loop, SLOT(quit()) );

        // ログアウト実行
        logoutSlot();

        // イベントループ開始
        loop.exec();
    }

    // ipconfigプロセスの終了を待つ
    m_ipconfig->waitForFinished();
}

void MainWidget::on_saveConfigButton_clicked()
{
    // save config button
    saveConfigSlot();
}

void MainWidget::on_logoutButton_clicked()
{
    // logout
    logoutSlot();

    // disable timer
    timer.stop();
    statusTimer.stop();

    ui->stopTimerButton->setEnabled( false );
}

void MainWidget::on_minimizePushButton_clicked()
{
    minimizeSlot();
}

void MainWidget::on_checkIpButton_clicked()
{
    // check ip
    checkIpSlot();
}

void MainWidget::on_renewIPButton_clicked()
{
    // IPを再取得
    renewIP();
}
