#include "mainwidget.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    const QString trFilename = "qautologin_";
    QString locale = QLocale::system().name();
    QTranslator translator;

    bool rc = translator.load( trFilename + locale );

    // failed load tr
    if ( !rc ) {
        qDebug() << "can't open tr file : " + trFilename + locale;
    } else {
        a.installTranslator( &translator );
    }

    MainWidget w;
    w.show();
    
    return a.exec();
}
