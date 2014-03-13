#include "mainwidget.h"
#include "ui_mainwidget.h"

MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWidget)
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
        timer.setInterval( ui->repatSpinBox->value() * 60 * 1000 );

        // set progress
        ui->progressBar->setMaximum( ui->repatSpinBox->value() );
        ui->progressBar->setValue( ui->repatSpinBox->value() );

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

    // if need id,pass
    if ( ui->saveIdCheckBox->isChecked() ) {
        ui->idEdit->setText( setting->value( "ID" ).toString() );
    }

    if ( ui->savePassCheckBox->isChecked() ) {
        ui->passEdit->setText( setting->value( "Pass" ).toString() );
    }

    // do setting
    if ( ui->autoStartCheckBox->isChecked() ) {
        // Auto Login
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
