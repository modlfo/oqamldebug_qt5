#include <QApplication>

#include "mainwindow.h"
#include "options.h"

#ifdef Q_WS_X11
#include <QProcess>
#include <stdio.h>
#include <X11/Xlib.h> 
#endif

static void MessageOutput( QtMsgType type, const char *msg )
{
  switch ( type ) 
  {
#if !QT_NO_DEBUG
    case QtDebugMsg:
      fprintf( stderr, "%s\n", msg );
      break;
    case QtSystemMsg:
      fprintf( stderr, "System Message: %s\n", msg );
      break;
    case QtWarningMsg:
      fprintf( stderr, "Warning: %s\n", msg );
      break;
#endif
    case QtFatalMsg:
      fprintf( stderr, "Fatal Error: %s\n", msg );
      abort();
    default:
       break;
  }
}


int main( int argc, char *argv[] )
{
    Q_INIT_RESOURCE( oqamldebug );
    qInstallMsgHandler( MessageOutput );

    Options::read_options();
#ifdef Q_WS_X11
    if (XOpenDisplay(NULL) == 0)
        return 1;
#endif
    QApplication app( argc, argv );

    QStringList arguments;
    for (int i=1; i< argc ; i++)
        arguments << argv[i];

    MainWindow mainWin(arguments);
    mainWin.show();
    int ret = app.exec();

    Options::save_options();

    return ret;
}
