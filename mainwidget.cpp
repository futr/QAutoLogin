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

    // create manager
    manager = new QNetworkAccessManager( this );

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

    // class no zikkenn public toka
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
    // login slot
    QString send_data;

    // AccessManager mode
    QNetworkRequest request;

    // clear counter
    ui->progressBar->setValue( timer.interval() / 1000 / 60 );
    ui->progressBar->setMaximum( timer.interval() / 1000 / 60 );

    send_data = "uid=" + ui->idEdit->text() + "&pwd=" + ui->passEdit->text()
              + "&Submit1=%E3%83%AD%E3%82%B0%E3%82%A4%E3%83%B3";

    // set url
    request.setUrl( QUrl( ui->serverNameEdit->text() + "cgi-bin/Login.cgi" ) );
    request.setRawHeader( "Content-type", "application/x-www-form-urlencoded" );

    // get
    QNetworkReply *reply = manager->post( request, send_data.toUtf8() );

    // connect
    connect( reply, SIGNAL(finished()), this, SLOT(readyReadSlot()) );
    connect( reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrorSlot(QList<QSslError>)) );
    connect( reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkErrorSlot(QNetworkReply::NetworkError)) );
}

void MainWidget::logoutSlot()
{
    // logout slot
    QString send_data;

    // AccessManager mode
    QNetworkRequest request;

    send_data = "Submit2=%E3%83%AD%E3%82%B0%E3%82%A2%E3%82%A6%E3%83%88";

    // set url
    request.setUrl( QUrl( ui->serverNameEdit->text() + "cgi-bin/Logout.cgi" ) );
    request.setRawHeader( "Content-type", "application/x-www-form-urlencoded" );

    // get
    QNetworkReply *reply = manager->post( request, send_data.toUtf8() );

    // connect
    connect( reply, SIGNAL(finished()), this, SLOT(readyReadSlot()) );
    connect( reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrorSlot(QList<QSslError>)) );
    connect( reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkErrorSlot(QNetworkReply::NetworkError)) );
}

void MainWidget::readyReadSlot()
{
    // ready read slot
    QNetworkReply *reply = qobject_cast< QNetworkReply *>( sender() );

    // バッファに読み込み
    QByteArray buffer( reply->readAll() );

    // パーサー作成
    QXmlStreamReader reader( buffer );

    // analyze html
    QString loginTime;
    QString logoutTime;
    LoginStatus stat = lsFailed;

    while ( !reader.atEnd() ) {
        // 次の要素を読む
        reader.readNext();

        // タグ開始
        if ( reader.isStartElement() ) {
            if ( reader.name() == "span" && reader.attributes().value( "id" ).toString() == "loginTime" ) {
                // ログイン時間取得
                loginTime = reader.readElementText();

                // ログイン成功
                stat = lsLogin;
            } else if ( reader.name() == "span" && reader.attributes().value( "id" ).toString() == "logoutTime" ) {
                // ログアウト時間取得
                logoutTime = reader.readElementText();
            } else if ( reader.name() == "p" && reader.attributes().value( "class" ).toString() == "subcaption" ) {
                // ログインかログアウトか判別
                QString mode = reader.readElementText();

                if ( mode == "Logout" || mode == "ログアウト" ) {
                    stat = lsLogout;
                }
            }
        }
    }

    // メッセージ作成
    QString info;
    QString title;

    if ( stat == lsLogin ) {
        // ログイン成功
        title = tr( "Login" );
        info  = QString() + tr( "Login time\t : " ) + loginTime + tr( "\nLogout time\t : " ) + logoutTime;

        if ( trayIcon->isVisible() ) {
            trayIcon->showMessage( title, info );
        }
    } else if ( stat == lsLogout ) {
        // logout complete
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

    // delete reply
    reply->deleteLater();
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
