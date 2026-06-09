#include "MainWindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    // High DPI scaling is enabled by default in Qt 6
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    a.setWindowIcon(QIcon(":/images/logo.png"));

    bool startMinimized = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--startup") == 0) {
            startMinimized = true;
            break;
        }
    }

    MainWindow w(startMinimized);
    if (!startMinimized) {
        w.show();
    }
    
    return a.exec();
}
