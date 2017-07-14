#include "radeon_profile.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    qDebug() << "Creating application object";

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

    qDebug() << "Creating radeon_profile";
    radeon_profile w;
    w.show();

    return a.exec();
}
