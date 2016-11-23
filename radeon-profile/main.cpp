#include "radeon_profile.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator(&a);
    if(translator.load(QLocale(), "strings", ".")
            || translator.load(QLocale(), "strings", ".", QApplication::applicationDirPath())
            || translator.load(QLocale(), "strings", ".", "/usr/share/radeon-profile"))
        a.installTranslator(&translator);
    else
        qWarning() << "Failed loading translation";

    radeon_profile w(a.arguments());
    
    return a.exec();
}
