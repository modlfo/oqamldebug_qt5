#ifndef ___options_H
#define ___options_H
#include <qstring.h>
#include <QStringList>
#include <QByteArray>
#include <QMdiArea>
#include <QWidget>
#include <QMainWindow>
#include <QWorkspace>
#include <QList>
class MainWindow ;

class Options 
{
  public:
     static QString         read_options ();
     static void            save_options ();
     static void            restore_window_position(MainWindow *wmain_p);
     static void            save_window_position(const QMdiArea *widget_p);
     static void            restore_window_position(const QString &name,QMainWindow *widget_p);
     static void            save_window_position(const QString &name,const QMainWindow *widget_p);
     static void            restore_window_position(const QString &name,QWidget *widget_p);
     static void            save_window_position(const QString &name,const QWidget *widget_p);
     static double          get_opt_double (const QString&,double def=0.0);
     static long            get_opt_long (const QString&,long def=0);
     static int             get_opt_int (const QString&,int def=0);
     static bool            get_opt_bool (const QString&,bool def=false);
     static QString         get_opt_str (const QString&,const QString &def=QString::null);
     static QByteArray      get_opt_array (const QString&,const QByteArray &def=QByteArray());
     static QStringList     get_opt_strlst (const QString&,const QStringList &def=QStringList());
     static void            set_opt (const QString&, double);
     static void            set_opt (const QString&, long);
     static void            set_opt (const QString&, int);
     static void            set_opt (const QString&, bool);
     static void            set_opt (const QString&,const  QString &);
     static void            set_opt (const QString&,const  QStringList &);
     static void            set_opt (const QString&,const  QByteArray &);
};
#endif
