#include <QApplication>

#include "mainwindow.h"
#include "options.h"

#ifdef Q_WS_X11
#include <QProcess>
#include <stdio.h>
#include <X11/Xlib.h> 
#endif


int main( int argc, char *argv[] )
{
    Q_INIT_RESOURCE( oqamldebug );

    Options::read_options();
    QStringList arguments;
    for (int i=1; i< argc ; i++)
        arguments << argv[i];

#ifdef Q_WS_X11
    if (XOpenDisplay(NULL) == 0)
        return 1;
#endif
    QApplication app( argc, argv );
    MainWindow mainWin(arguments);
    mainWin.show();
    int ret = app.exec();

    Options::save_options();

    return ret;
}
