#include "options.h"
#include "ocamlsource.h"
#include "mainwindow.h"
#include <qsettings.h>
#include <qworkspace.h>
#include <qwidget.h>
#include <qtextstream.h>
#include <QMainWindow>
#include <QMdiSubWindow>
#include <QMdiArea>
#define MAX_UNCLEANED_EXITS 5

static long nb_uncleaned_exits = -1;
static QSettings *settings_p = NULL;
static QString option( const QString &opt )
{
    QString         opt_str;
    opt_str = opt;
    opt_str.remove( '/' );
    return opt_str;
}

static bool option_valid()
{
    if ( nb_uncleaned_exits < 0 )
        return false;

    return ( MAX_UNCLEANED_EXITS >= nb_uncleaned_exits ) ;
}

QString Options::read_options ()
{
    QString         opt_str = option( "NB_UNCLEANED_EXITS" );
    if ( settings_p )
        delete settings_p;
    settings_p = new QSettings( QSettings::IniFormat, QSettings::UserScope, "oqaml", "oqamldebug" );
#if NO_DEBUG
    nb_uncleaned_exits = 1 + settings_p->value ( opt_str, 0 ).toInt();
#else
    nb_uncleaned_exits = 0;
#endif
    settings_p->setValue ( opt_str, ( int ) nb_uncleaned_exits );
    delete settings_p;
    settings_p = new QSettings( QSettings::IniFormat, QSettings::UserScope, "oqaml", "oqamldebug" );
    return settings_p->fileName();
}


void Options::save_options ()
{
    set_opt( "NB_UNCLEANED_EXITS", ( long )0 );
    delete          settings_p;
    settings_p = NULL;
}


double Options::get_opt_double ( const QString &opt, double def )
{
    double            val;
    QString         opt_str = option( opt );
    if ( !option_valid() )
        return def;

    if ( settings_p )
    {
        val = settings_p->value ( opt_str, def ).toDouble();
        return val;
    }
    else
        return def;
}

int Options::get_opt_int ( const QString &opt, int def )
{
    return get_opt_long( opt, def );
}

long Options::get_opt_long ( const QString &opt, long def )
{
    long            val;
    QString         opt_str = option( opt );
    if ( !option_valid() )
        return def;

    if ( settings_p )
    {
        val = settings_p->value ( opt_str, static_cast<int>( def ) ).toInt();
        return val;
    }
    else
        return def;
}

bool Options::get_opt_bool ( const QString &opt, bool def )
{
    bool            result;
    QString         opt_str = option( opt );
    if ( !option_valid() )
        return def;

    if ( settings_p )
    {
        result = settings_p->value ( opt_str, def ).toBool();
        return result;
    }
    else
        return def;
}


QList<int> Options::get_opt_intlst ( const QString &opt, const QList<int> &def )
{
    QStringList defs;
    for (QList<int>::const_iterator itDef = def.begin(); itDef != def.end(); ++itDef )
        defs << QString::number( *itDef );
    QStringList rets = get_opt_strlst( opt, defs );

    QList<int> ret;
    for (QStringList::const_iterator itStr = rets.begin(); itStr != rets.end(); ++itStr )
        ret << itStr->toInt();

    return ret;
}

QStringList Options::get_opt_strlst ( const QString &opt, const QStringList &def )
{
    QString         opt_str = option( opt );
    if ( !option_valid() )
        return def;

    if ( settings_p )
    {
        QStringList ret = settings_p->value ( opt_str, def ).toStringList();
        return ret;
    }
    else
        return def;
}

QByteArray Options::get_opt_array ( const QString &opt, const QByteArray &def )
{
    QString         opt_str = option( opt );
    if ( !option_valid() )
        return def;

    if ( settings_p )
    {
        QString ret = settings_p->value ( opt_str, def ).toString();
        return QByteArray::fromHex(ret.toLatin1());
    }
    else
        return def;
}

QString Options::get_opt_str ( const QString &opt, const QString &def )
{
    QString         opt_str = option( opt );
    if ( !option_valid() )
        return def;

    if ( settings_p )
    {
        QString ret = settings_p->value ( opt_str, def ).toString();
        return ret;
    }
    else
        return def;
}


void Options::set_opt ( const QString &opt, double val )
{
    QString         opt_str = option( opt );
    if ( settings_p )
        settings_p->setValue ( opt_str,  val );
}


void Options::set_opt ( const QString &opt, int val )
{
    set_opt( opt, static_cast<long>( val ) );
}

void Options::set_opt ( const QString &opt, long val )
{
    QString         opt_str = option( opt );
    if ( settings_p )
        settings_p->setValue ( opt_str, ( int ) val );
}


void Options::set_opt ( const QString &opt, const QByteArray &val )
{
    QString         opt_str = option( opt );
    if ( settings_p )
        settings_p->setValue ( opt_str, QString::fromLatin1(val.toHex()) );
}

void Options::set_opt ( const QString &opt, const QString &val )
{
    QString         opt_str = option( opt );
    if ( settings_p )
        settings_p->setValue ( opt_str, val );
}

void Options::set_opt ( const QString &opt, const QStringList &val )
{
    QString         opt_str = option( opt );
    if ( settings_p )
        settings_p->setValue ( opt_str, val );
}

void Options::set_opt ( const QString &opt, const QList<int> &val )
{
    QStringList vals;
    for (QList<int>::const_iterator itVal = val.begin(); itVal != val.end(); ++itVal )
        vals << QString::number( *itVal );
    set_opt( opt, vals );
}


void Options::set_opt ( const QString &opt, bool val )
{
    QString         opt_str = option( opt );
    if ( settings_p )
        settings_p->setValue ( opt_str, val );
}

void Options::restore_window_position( const QString &name, QMainWindow *widget_p )
{
    bool restore_pos = get_opt_bool( "RESTORE_WINDOW_POSITION", true );
    if ( !restore_pos )
        return ;

    restore_window_position( name, ( QWidget* )widget_p );
    widget_p->restoreState( get_opt_array( "WIDGET_POS_DOCKPOS" ) );
}

void Options::save_window_position( const QString &name, const QMainWindow *widget_p )
{
    save_window_position( name, ( QWidget* )widget_p );
    set_opt( "WIDGET_POS_DOCKPOS", widget_p->saveState() );
}

void Options::restore_window_position( const QString &name, QWidget *widget_p )
{
    bool maximized ;
    bool restore_pos;
    long  posx, posy, height, width ;

    restore_pos = get_opt_bool( "RESTORE_WINDOW_POSITION", true );
    if ( !restore_pos )
        return ;

    maximized = get_opt_bool( "WIDGET_POS_" + name + "_MAXIMIZED", false );
    posx = get_opt_long( "WIDGET_POS_" + name + "_X", 0 );
    posy = get_opt_long( "WIDGET_POS_" + name + "_Y", 0 );
    width = get_opt_long( "WIDGET_POS_" + name + "_WIDTH", 100 );
    height = get_opt_long( "WIDGET_POS_" + name + "_HEIGHT", 100 );

    if ( maximized )
        widget_p->showMaximized();
    else
    {
        QPoint pos( posx, posy );
        QSize s( width, height );
        widget_p->showNormal();
        widget_p->move( pos );
        widget_p->resize( s );
    }
}

void Options::save_window_position( const QString &name, const QWidget *widget_p )
{
    bool maximized ;
    long  posx, posy, height, width ;
    maximized = widget_p->isMaximized();
    posx = widget_p->x();
    posy = widget_p->y();

    height = widget_p->height();
    width = widget_p->width();

    set_opt( "WIDGET_POS_" + name + "_MAXIMIZED", maximized );
    set_opt( "WIDGET_POS_" + name + "_X", posx );
    set_opt( "WIDGET_POS_" + name + "_Y", posy );
    set_opt( "WIDGET_POS_" + name + "_WIDTH", width );
    set_opt( "WIDGET_POS_" + name + "_HEIGHT", height );
}


void Options::save_window_position( const QMdiArea *workspace_p )
{
    QList<QMdiSubWindow *>  windows = workspace_p->subWindowList( QMdiArea::StackingOrder );
    QMdiSubWindow *mdi_subwindow_p;
    long nb_widget = windows.count();
    set_opt( "WIDGET_NB_WIDGET", nb_widget );
    int id = 0;
    for ( QList<QMdiSubWindow *>::const_iterator it = windows.begin(); it != windows.end(); ++it )
    {
        mdi_subwindow_p = *it;
        QString name = "WORKSPACE" + QString::number( id );
        OCamlSource *sourceview_p = dynamic_cast<OCamlSource*>( mdi_subwindow_p->widget() );
        if ( sourceview_p )
        {
            set_opt( "TYPE_" + name, QString( "SOURCEVIEW" ) );
            QString source = sourceview_p->currentFile();
            set_opt( "SOURCE_" + name, source );
            set_opt( "SOURCE_USER_LOADED_" + name, sourceview_p->fromUserLoaded() );
            save_window_position( name, mdi_subwindow_p );
        }
        id++;
    }
}

void Options::restore_window_position( MainWindow *wmain_p )
{
    bool restore_pos = get_opt_bool( "RESTORE_WINDOW_POSITION", true );
    if ( !restore_pos )
        return ;
    long nb_widget = get_opt_long( "WIDGET_NB_WIDGET", 0 );
    for ( int i = 0; i < nb_widget; i++ )
    {
        QString name = "WORKSPACE" + QString::number( i );
        QString type = get_opt_str( "TYPE_" + name, QString() );
        if ( type == "SOURCEVIEW" )
        {
            QString source = get_opt_str( "SOURCE_" + name, QString::null ).toAscii();
            bool from_user_loaded = get_opt_bool( "SOURCE_USER_LOADED_" + name, false );
            QMdiSubWindow *widget_p = wmain_p->openOCamlSource( source , from_user_loaded );
            if ( widget_p )
            {
                restore_window_position( name, widget_p );
            }
        }
    }
}

