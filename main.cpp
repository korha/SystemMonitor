#include "systemmonitor.h"
#include <QtCore/QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTranslator translator;
    {
        const QString strAppName = a.applicationName();
        const QString strLang = QSettings("UserPrograms", strAppName).value("lang").toString();
        if (translator.load("lang_" + (strLang.isEmpty() ? QLocale::system().name() : strLang), strAppName + "_lang"))
            a.installTranslator(&translator);
    }
    bool bValid = false;
    SystemMonitor w(bValid);
    if (bValid)
    {
        w.show();
        w.lower();
        return a.exec();
    }
    return 0;
}
