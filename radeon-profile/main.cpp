#include "radeon_profile.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator * translator = new QTranslator();
    /* NOTE FOR PACKAGING
     * In this folder are present translation files ( strings.*.ts )
     * After editing the source code it is necessary to run "lupdate" in this folder (in QtCreator: Tools > External > Linguist > Update)
     * Then these files can be edited with the Qt linguist
     * When the translations are ready it is necessary to run "lrelease" in this folder (in QtCreator: Tools > External > Linguist > Release)
     * This produces compiled translation files ( strings.*.qm )
     * In order to be used these files need to be packaged together with the runnable
     * These can be placed in "/usr/share/radeon-profile/" or in the same folder of the binary (useful for development)
     */
    if(translator->load(QLocale(), "strings", ".", "/usr/share/radeon-profile") || translator->load(QLocale(), "strings", "."))
        a.installTranslator(translator);
    else
        qWarning() << "Failed loading translation";

    radeon_profile w(a.arguments());
    
    return a.exec();
}
