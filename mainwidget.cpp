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

    // setting ( if you )
    setting = new QSettings( "conf.ini", QSettings::IniFormat, this );

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

    // show to webkit
    ui->webView->setContent( reply->readAll() );

    // analyze html
    QWebElementCollection loginTime = ui->webView->page()->mainFrame()->documentElement().findAll( "span[id=loginTime]" );
    QWebElementCollection logoutTIme = ui->webView->page()->mainFrame()->documentElement().findAll( "span[id=logoutTime]" );
    QStringList htmlList = ui->webView->page()->mainFrame()->toPlainText().split( "\n" );

    // Make message
    if ( loginTime.count() ) {
        // login complete
        if ( trayIcon->isVisible() ) {
            trayIcon->showMessage( tr( "Login" ), QString() + tr( "Login time\t : " ) + loginTime.first().toPlainText() + tr( "\nLogout time\t : " ) + logoutTIme.first().toPlainText() );
        }
    } else if ( htmlList.indexOf( "ログアウトしました" ) != -1 ) {
        // logout complete
        if ( trayIcon->isVisible() ) {
            trayIcon->showMessage( tr( "Logout" ), tr( "Logout complete." ) );
        }
    } else {
        // error
        if ( trayIcon->isVisible() ) {
            trayIcon->showMessage( tr( "Error" ), tr( "Some error occurred." ), QSystemTrayIcon::Warning );
        }
    }

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

    foreach ( QSslError err, errors ) {
        errString += err.errorString() + "\n";
    }

    // Auto ignore
    reply->ignoreSslErrors( errors );

    // view ssl error
    if ( isVisible() ) {
        QMessageBox::warning( this, tr( "Ssl warning" ), errString + tr( "\n\nThese \"SslError\" are ignored." ) );
    }

    /*
    if ( QMessageBox::question( this, tr( "Ssl errors" ), errString, QMessageBox::Ignore, QMessageBox::Cancel ) == QMessageBox::Cancel ) {
        // cancel
        return;
    } else {
        // ignore
        reply->ignoreSslErrors( errors );
    }
    */

    // reply->deleteLater();
}

void MainWidget::trayIconClickSlot( QSystemTrayIcon::ActivationReason reason )
{
    // tray icon clicked
    if ( reason == QSystemTrayIcon::Trigger ) {
        this->show();
        trayIcon->hide();
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
