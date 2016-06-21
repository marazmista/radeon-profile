#include "radeon_profile.h"
#include <QApplication>
#include <QTranslator>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator(&a);
    if (translator.load(QLocale(), "strings", ".")){
        qDebug() << "Translation found in in the current path";
        a.installTranslator(&translator);
    } else if (translator.load(QLocale(), "strings", ".", QApplication::applicationDirPath())){
        qDebug() << "Translation found in in the executable directory";
        a.installTranslator(&translator);
    } else if (translator.load(QLocale(), "strings", ".", "/usr/share/radeon-profile")){
        qDebug() << "Translation found in /usr/share/radeon-profile";
        a.installTranslator(&translator);
    } else
        qWarning() << "Failed loading translation";

    radeon_profile w(a.arguments());
    
    return a.exec();
}
