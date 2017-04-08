#include "testchrono.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TestChrono w;
    w.show();

    return a.exec();
}
