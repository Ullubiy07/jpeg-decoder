#include "ui/MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    MainWindow window;
    const QStringList arguments = app.arguments();
    if (arguments.size() > 1 && !window.openFile(arguments.at(1))) {
        return 1;
    }
    window.show();
    return app.exec();
}
