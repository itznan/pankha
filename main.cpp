#include "MainWindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    // High DPI scaling is enabled by default in Qt 6
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/logo.png"));

    MainWindow w;
    w.show();
    
    return a.exec();
}
