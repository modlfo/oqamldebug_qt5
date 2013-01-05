#ifndef OCAMLDEBUG_H
#define OCAMLDEBUG_H

#include <QPlainTextEdit>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include "ocamldebughighlighter.h"
#include "filesystemwatcher.h"
#include "breakpoint.h"
#include "debuggercommand.h"
#include "arguments.h"

class OCamlDebugTime;
class OCamlRun;

class OCamlDebug : public QPlainTextEdit
{
    Q_OBJECT

public:
    OCamlDebug( QWidget *parent_p , OCamlRun *ocamlrun, const QString &ocamldebug, const Arguments &arguments, const QString & init_script );
    virtual ~OCamlDebug( );
    void setArguments(const Arguments &arguments );
    void setOCamlDebug(const QString &);
    int debugTimeAreaWidth();
    void debugTimeAreaPaintEvent( QPaintEvent *event );
    const QMap<int,int> & timeInfo() const { return _time_info; }
    void setInitializationScript( const QString &s ) { _ocamldebug_init_script =s ; }
    const BreakPoints & breakpoints() const { return _breakpoints; }

protected:
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);

    virtual void keyPressEvent ( QKeyEvent * e );
    virtual void keyReleaseEvent ( QKeyEvent * e );

public slots:
    void startDebug();
    void stopDebug();
    void debuggerInterrupt();
    void debugger( const DebuggerCommand & command );
    void updateDebugTimeAreaWidth(int newBlockCount);
    void updateDebugTimeArea(const QRect &, int);

private slots:
    void displayAllCommands(bool) ;
    void receiveDataFromProcessStdOutput();
    void receiveDataFromProcessStdError();
    void fileChanged ( );

signals:
    void stopDebugging( const QString &, int , int , bool);
    void breakPointList( const BreakPoints & );
    void breakPointHit( const QList<int> & );
    void debuggerStarted( bool );
    void debuggerCommand( const QString & command, const QString & result );
private:
    void restoreBreakpoints();
    void saveBreakpoints();
    void processOneQueuedCommand();
    void contextMenuEvent(QContextMenuEvent *event);
    void wheelEvent ( QWheelEvent * event );
    void saveLRU(const QString &command);
    void startProcess( );
    void clear();
    void readChannel();
    void appendText(const QByteArray &);
    OCamlDebugHighlighter *highlighter;
    QProcess *process_p;
    QRegExp emacsLineInfoRx ;
    BreakPoints _breakpoints;
    QRegExp readyRx ;
    QRegExp deleteBreakpointRx ;
    QRegExp hitBreakpointRx ;
    QRegExp hitBreakpointIdRx ;
    QRegExp newBreakpointRx ;
    QRegExp emacsHaltInfoRx ;
    QRegExp timeInfoRx ;
    QRegExp ocamlrunConnectionRx ;
    QList<QRegExp> _debuggerOutputsRx;
    QString _ocamldebug, _ocamldebug_init_script;
    Arguments _arguments;
    QString _command_line,_command_line_last,_command_line_backup;
    QMap<int,int> _time_info;
    int _time;
    int _cursor_position;
    int _lru_position;
    void displayCommandLine();
    void undisplayCommandLine();
    QStringList _lru;
    FileSystemWatcher *file_watch_p;
    QList<DebuggerCommand> _command_queue;
    OCamlDebugTime *debugTimeArea;
    QList<int> _breakpoint_hits;
    QStringList generateBreakpointCommands() const;
    OCamlRun *_ocamlrun_p;
    const int _port_min, _port_max;
    int _current_port;
    int findFreeServerPort( int ) const;
    bool _display_all_commands;
};

class OCamlDebugTime : public QWidget
{
    public:
        OCamlDebugTime( OCamlDebug *d ) ;
        QSize sizeHint() const;

    protected:
        void paintEvent( QPaintEvent *event );
        void mouseDoubleClickEvent ( QMouseEvent * event );
        bool event(QEvent *event);

    private:
        OCamlDebug *debugger;
};

#endif
