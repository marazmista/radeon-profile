#include "radeon_profile.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    radeon_profile w(a.arguments());
    
    return a.exec();
}
