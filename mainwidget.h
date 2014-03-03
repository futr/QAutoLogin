#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QtNetwork/QTcpSocket>
#include <QMessageBox>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QCloseEvent>
#include <QWebFrame>
#include <QWebElement>
#include <QStringList>
#include <QNetworkAddressEntry>
#include <QSslError>

namespace Ui {
class MainWidget;
}

class MainWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit MainWidget(QWidget *parent = 0);
    ~MainWidget();
    
private slots:
    void on_loginButton_clicked();

    void loginSlot();
    void logoutSlot();
    void readyReadSlot();
    void saveConfigSlot();
    void loadConfigSlot();
    void minimizeSlot();
    void updateStatusSlot();
    void clearStatusSlot();
    void sslErrorSlot( const QList<QSslError> & errors );
    void trayIconClickSlot( QSystemTrayIcon::ActivationReason reason );
    void networkErrorSlot( QNetworkReply::NetworkError code );
    void checkIpSlot();

    void on_aboutButton_clicked();

    void on_closeButton_clicked();

    void on_stopTimerButton_clicked();

    void on_saveConfigButton_clicked();

    void on_logoutButton_clicked();

    void on_minimizePushButton_clicked();

    void on_checkIpButton_clicked();

private:
    void createTrayIcon();
    void createAction();

protected:
    virtual void closeEvent(QCloseEvent *);

private:
    Ui::MainWidget *ui;

    QTcpSocket socket;
    QTimer timer;
    QTimer statusTimer;

    QNetworkAccessManager *manager;

    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;

    QAction *closeAction;
    QAction *connectAction;
    QAction *disconnectAction;
    QAction *showAction;

    QSettings *setting;
};

#endif // MAINWIDGET_H
