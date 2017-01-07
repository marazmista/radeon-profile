#include "radeon_profile.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
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
    
    return a.exec();
}
