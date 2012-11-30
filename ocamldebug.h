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

class OCamlDebugTime;

class OCamlDebug : public QPlainTextEdit
{
    Q_OBJECT

public:
    OCamlDebug( QWidget * parent_p, const QString &, const QStringList &);
    virtual ~OCamlDebug( );
    void setArguments(const QStringList &);
    void setOCamlDebug(const QString &);
    int debugTimeAreaWidth();
    void debugTimeAreaPaintEvent( QPaintEvent *event );
    const QMap<int,int> & timeInfo() const { return _time_info; }

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
    void processOneQueuedCommand();
    void contextMenuEvent(QContextMenuEvent *event);
    void wheelEvent ( QWheelEvent * event );
    void saveLRU(const QString &command);
    void startProcess( const QString &program , const QStringList &arguments );
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
    QString _ocamldebug;
    QString _ocamlapp;
    QStringList _arguments;
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
