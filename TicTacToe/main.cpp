#include <QtWidgets/QApplication>

#include "TMainWindow.h"
#include "TApp.h"


int main(int argc, char *argv[])
{
    //create the qapp
    TApp a(argc, argv);

    //create the main window
    TMainWindow w;
    w.show();

    //go go go!
    return a.exec();
}
