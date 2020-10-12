#include "httpdownload.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    HttpDownload w;
    w.setWindowTitle("Git Repository Checker");
    w.show();

    return a.exec();
}
