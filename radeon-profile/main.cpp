#include "radeon_profile.h"
#include <QApplication>
#include <QTranslator>
#include <QSplashScreen>
#include <QPixmap>
#include <QBitmap>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QPixmap pm(":/icon/extra/radeon-profile.png");
    QSplashScreen loading(pm);
    loading.setMask(pm.mask());
	loading.show();

    QTranslator translator;
    QLocale locale;

    if (locale.language() != QLocale::Language::English) {

        if (translator.load(locale, "strings", ".")
                || translator.load(locale, "strings", ".", QApplication::applicationDirPath())
                || translator.load(locale, "strings", ".", "/usr/share/radeon-profile"))

            a.installTranslator(&translator);
        else
            qWarning() << "Translation not found.";
    }

    radeon_profile w(a.arguments());
	loading.close();
    
    return a.exec();
}
