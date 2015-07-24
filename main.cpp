#include "systemmonitor.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SystemMonitor w;
    if (w.isValid())
    {
        w.show();
        w.lower();
        return a.exec();
    }
    return 0;
}
