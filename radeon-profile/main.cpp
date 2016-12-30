#include "radeon_profile.h"
#include <QApplication>
#include <QTranslator>
#include <QSplashScreen>
#include <QPixmap>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

	QSplashScreen loading(QPixmap(":/icon/extra/radeon-profile.png"));
	loading.show();

    QTranslator translator(&a);
    if(translator.load(QLocale(), "strings", ".")
            || translator.load(QLocale(), "strings", ".", QApplication::applicationDirPath())
            || translator.load(QLocale(), "strings", ".", "/usr/share/radeon-profile"))
        a.installTranslator(&translator);
    else
        qWarning() << "Failed loading translation";

    radeon_profile w(a.arguments());
	loading.close();
    
    return a.exec();
}
