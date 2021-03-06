#include <QtGui>
#include <QtDebug>
#include <QTcpServer>
#include <QMessageBox>
#include <QToolTip>
#include <QMenu>
#include "ocamldebughighlighter.h"
#include "ocamldebug.h"
#include "ocamlrun.h"
#include "options.h"
#if !defined(Q_OS_WIN32)
#include <sys/types.h>
#include <signal.h>
#endif


OCamlDebug::OCamlDebug( QWidget *parent_p , OCamlRun *ocamlrun_p, const QString &ocamldebug, const Arguments &arguments, const QString & init_script ) : QPlainTextEdit(parent_p),
    emacsLineInfoRx("^\\x001A\\x001AM([^:].[^:]*):([^:]*):([^:]*):([^:\\n]*)\\n*$") ,
    readyRx("^\\(ocd\\) *") ,
    deleteBreakpointRx("^Removed breakpoint ([0-9]+) at [0-9]+ *: .*$"),
    hitBreakpointRx("^Breakpoints? :( [0-9]+)+ ?\\n?$"),
    hitBreakpointIdRx("( [0-9]+)"),
    newBreakpointRx("^Breakpoint ([0-9]+) at [0-9]+ *: file ([^,]*), line ([0-9]+), characters ([0-9]+)-([0-9]+).*$"),
    emacsHaltInfoRx("^\\x001A\\x001AH.*$"),
    timeInfoRx("^Time *: *([0-9]+)( - pc *: *([0-9]+) - .*)?\\n?$"),
    ocamlrunConnectionRx("^Waiting for connection\\.\\.\\.\\(the socket is [a-z.0-9_A-Z]*:[0-9]+\\)\\n?$"),
    _ocamldebug_init_script( init_script ),
    _arguments( arguments ),
    _ocamlrun_p( ocamlrun_p ),
    _port_min( 18000 ),
    _port_max( 18999 )
{
    _current_port = Options::get_opt_int( "OCAMLDEBUG_PORT", _port_min ) ;
    _debuggerOutputsRx.append( QRegExp( "^No such frame\\.\\n?$" ) );
    _debuggerOutputsRx.append( QRegExp( "^#([0-9]+)  *Pc *: *[0-9]+ .*$" ) );
    _debuggerOutputsRx.append( QRegExp( "^Loading program\\.\\.\\.[\\n ]+$" ) );
    _debuggerOutputsRx.append( QRegExp( "^done\\.[\\n ]+$" ) );
    _display_all_commands = Options::get_opt_bool( "DISPLAY_ALL_OCAMLDEBUG_COMMANDS", false );

    file_watch_p = NULL;
    debugTimeArea = new OCamlDebugTime( this );
    connect( this, SIGNAL( blockCountChanged( int ) ), this, SLOT( updateDebugTimeAreaWidth( int ) ) );
    connect( this, SIGNAL( updateRequest( QRect, int ) ), this, SLOT( updateDebugTimeArea( QRect, int ) ) );

    updateDebugTimeAreaWidth( 0 );
    emit debuggerStarted( false );
    setEnabled( false );
    _lru = Options::get_opt_strlst ("OCAMLDEBUG_COMMANDS");
    setReadOnly( false );
    setUndoRedoEnabled( false );
    setAttribute(Qt::WA_DeleteOnClose);
    highlighter = new OCamlDebugHighlighter(this->document());
    process_p = NULL;
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    setFont(font);
    setArguments( _arguments );
    setOCamlDebug( ocamldebug );

    setCursorWidth(3);
}

OCamlDebug::~OCamlDebug()
{
    emit debuggerStarted( false );
    disconnect() ;
    clear();
    if ( file_watch_p )
        delete file_watch_p;
}

void OCamlDebug::clear()
{
    _command_line.clear();
    _command_line_last.clear();
    _cursor_position=0;
    _lru_position = -1 ;
    if ( process_p )
    {
        process_p->terminate();
        if (process_p->waitForFinished( 1000 ) )
            process_p->kill();
        process_p->close();
        delete process_p;
        process_p = NULL;
        setEnabled( false );
    }
    QPlainTextEdit::clear();
}

void OCamlDebug::setOCamlDebug( const QString &ocamldebug )
{
    _ocamldebug = ocamldebug ;
}

void OCamlDebug::setArguments( const Arguments &arguments )
{
    _arguments = arguments ;
    QString ocamlapp = _arguments.ocamlApp() ;
    if ( file_watch_p )
        delete file_watch_p;
    file_watch_p = new FileSystemWatcher( ocamlapp );
    connect ( file_watch_p , SIGNAL( fileChanged ( ) ) , this , SLOT( fileChanged () ) , Qt::QueuedConnection );
}

void OCamlDebug::fileChanged ( )
{
    QTextCursor cur = textCursor();
    cur.movePosition(QTextCursor::End, QTextCursor::MoveAnchor) ;
    cur.insertText("\n"+tr("Application %1 is modified.").arg( _arguments.ocamlApp() )+"\n");
    stopDebug();
    startDebug();
}

void OCamlDebug::startDebug()
{
    startProcess ( );
}

void OCamlDebug::stopDebug()
{
    clear();
    QTextCursor cur = textCursor();
    cur.movePosition(QTextCursor::End, QTextCursor::MoveAnchor) ;
    cur.insertText("\n"+tr("OCamlDebug process stopped.")+"\n");
}

void OCamlDebug::closeEvent(QCloseEvent *event)
{
    event->accept();
}

void OCamlDebug::keyPressEvent ( QKeyEvent * e )
{
     if (process_p)
     {
         switch (e->key())
         {
             case Qt::Key_PageDown: 
             case Qt::Key_PageUp: 
                 QPlainTextEdit::keyPressEvent ( e );
                 break;
             case Qt::Key_Delete: 
                 {
                     if (!_command_line.isEmpty() && _cursor_position>0)
                     {
                         QString command_line = _command_line.left(_cursor_position) + _command_line.mid(_cursor_position+1);
                         _command_line = command_line;
                         displayCommandLine();
                     }
                 }
                 break;
             case Qt::Key_Backspace: 
                 {
                     if (!_command_line.isEmpty() && _cursor_position>0)
                     {
                         QString command_line = _command_line.left(_cursor_position-1) + _command_line.mid(_cursor_position);
                         _command_line = command_line;
                         _cursor_position--;
                         displayCommandLine();
                     }
                 }
                 break;
             case Qt::Key_Up: 
                 {
                     if ( e->modifiers() & Qt::ShiftModifier)
                     {
                         QPlainTextEdit::keyPressEvent ( e );
                         break;
                     }
                     if (_lru.isEmpty())
                         break;
                     
                     if ( _lru_position < 0 )
                     {
                         _command_line_backup = _command_line;
                         _lru_position = _lru.size() - 1 ;
                     }
                     else if ( _lru_position == 0)
                         break;
                     else
                         _lru_position--;
                     _command_line = _lru.at( _lru_position );

                     _cursor_position = _command_line.length();
                     displayCommandLine();
                 }
                 break;
             case Qt::Key_Down: 
                 {
                     if ( e->modifiers() & Qt::ShiftModifier)
                     {
                         QPlainTextEdit::keyPressEvent ( e );
                         break;
                     }
                     if (_lru.isEmpty())
                         break;
                     if ( _lru_position < 0 )
                         break;
                     else if ( _lru_position == _lru.size()-1)
                     {
                         _lru_position = -1;
                         _command_line = _command_line_backup;
                     }
                     else
                     {
                         _lru_position++;
                         _command_line = _lru.at( _lru_position );
                     }

                     _cursor_position = _command_line.length();
                     displayCommandLine();
                 }
                 break;
             case Qt::Key_Right: 
                 {
                     _cursor_position++;
                     if (_cursor_position > _command_line.length())
                         _cursor_position = _command_line.length();
                     else
                         displayCommandLine();
                 }
                 break;
             case Qt::Key_Left: 
                 {
                     _cursor_position--;
                     if (_cursor_position < 0)
                         _cursor_position = 0;
                     else
                         displayCommandLine();
                 }
                 break;
             case Qt::Key_Return: 
                 {
                     if ( _command_line.isEmpty())
                     {
                         if ( _lru.isEmpty() )
                             break;
                         _command_line =  _lru.last();
                         _cursor_position = _command_line.length();
                     }
                     debugger( DebuggerCommand( _command_line, DebuggerCommand::IMMEDIATE_COMMAND ) );
                     _command_line.clear();
                     _command_line_last.clear();
                     _cursor_position=0;
                     _lru_position = -1 ;
                 }
                 break;
             case Qt::Key_C: 
                 if (e->modifiers() == Qt::ControlModifier)
                 {
                     debuggerInterrupt();
                     break;
                 }
             default:
                 {
                     QString command_line = _command_line.left(_cursor_position) + e->text() + _command_line.mid(_cursor_position);
                     _command_line = command_line;
                     _cursor_position++;
                     displayCommandLine();
                 }
                 break;
         }
     }
     else
         QPlainTextEdit::keyPressEvent ( e );
}

void OCamlDebug::undisplayCommandLine()
{
    if ( !_command_line_last.isEmpty() )
    {
        QTextCursor cur = textCursor();
        cur.movePosition(QTextCursor::End, QTextCursor::MoveAnchor) ;
        cur.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, _command_line_last.length()) ;
        cur.removeSelectedText();
        cur.movePosition(QTextCursor::End, QTextCursor::MoveAnchor) ;
    }
    _command_line_last=_command_line;
}

void OCamlDebug::displayCommandLine()
{
    undisplayCommandLine();
    if ( !_command_line.isEmpty() )
    {
        QTextCursor cur = textCursor();
        cur.movePosition(QTextCursor::End, QTextCursor::MoveAnchor) ;
        cur.insertText(_command_line);
        cur.movePosition(QTextCursor::End, QTextCursor::MoveAnchor );
        int cursor_pos = _command_line.length() - _cursor_position;
        if (cursor_pos>0)
            cur.movePosition(QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor, cursor_pos) ;
        setTextCursor(cur);
        ensureCursorVisible();
    }
    else
    {
        QTextCursor cur = textCursor();
        cur.movePosition(QTextCursor::End, QTextCursor::MoveAnchor );
        setTextCursor(cur);
    }
}

void OCamlDebug::keyReleaseEvent ( QKeyEvent * e )
{
    QPlainTextEdit::keyReleaseEvent ( e );
}

void OCamlDebug::startProcess()
{
    clear();
    _current_port = findFreeServerPort( _current_port );
    if ( _current_port == 0 )
    {
        QMessageBox::warning(this, tr("OCamlDebug server"),
                tr("No free TCP port found between %1 and %2.").arg( _port_min ).arg( _port_max ),
                QMessageBox::Ok );
    }
    else
        Options::set_opt( "OCAMLDEBUG_PORT", _current_port );
    _time_info.clear();
    _time = -1 ;
    _command_queue.clear();
    _command_queue << DebuggerCommand( "", DebuggerCommand::IMMEDIATE_COMMAND ) ;
    QString program = _ocamldebug ;
    QStringList args;
    args 
        << _arguments.ocamlDebugArguments() 
        << _arguments.ocamlApp() 
        << _arguments.ocamlAppArguments() 
        ;

    process_p =  new QProcess(this) ;
    process_p->setProcessChannelMode(QProcess::MergedChannels);
    connect ( process_p , SIGNAL( readyReadStandardOutput() ) , this , SLOT( receiveDataFromProcessStdOutput()) );
    connect ( process_p , SIGNAL( readyReadStandardError() ) , this , SLOT( receiveDataFromProcessStdError()) );
    process_p->start( program , args );
    if ( ! process_p->waitForStarted())
    {
        QMessageBox::warning( this , tr( "Error Executing Command" ) , program );
        clear();
        return;
    }

    _ocamlrun_p->setArguments( _arguments );
    debugger( DebuggerCommand( "set loadingmode manual", DebuggerCommand::HIDE_ALL_OUTPUT ) );
    debugger( DebuggerCommand( "set socket 127.0.0.1:"+QString::number( _current_port ), DebuggerCommand::HIDE_DEBUGGER_OUTPUT ) );
    debugger( DebuggerCommand( "goto 0", DebuggerCommand::HIDE_ALL_OUTPUT ) );
    if ( QFile::exists( _ocamldebug_init_script ) )
        debugger( DebuggerCommand( "source " + _ocamldebug_init_script, DebuggerCommand::SHOW_ALL_OUTPUT ) );
    restoreBreakpoints();
    emit debuggerStarted( true );
    setEnabled( true );
}

void OCamlDebug::saveBreakpoints()
{
    Options::set_opt( "BREAKPOINT_COMMANDS", generateBreakpointCommands() );
}

void OCamlDebug::restoreBreakpoints()
{
    QStringList breakpoint_commands = Options::get_opt_strlst( "BREAKPOINT_COMMANDS" );
    _breakpoints.clear();
    saveBreakpoints();
    emit breakPointList( _breakpoints );

    for (QStringList::const_iterator itCommand = breakpoint_commands.begin(); itCommand != breakpoint_commands.end(); ++itCommand )
        debugger( DebuggerCommand( *itCommand, DebuggerCommand::HIDE_ALL_OUTPUT) );

    if ( _breakpoints.isEmpty() && !breakpoint_commands.isEmpty() )
    { // if resetting breakpoints are not possible, do not save the new breakpoint list
        Options::set_opt( "BREAKPOINT_COMMANDS", breakpoint_commands );
    }
}

QStringList OCamlDebug::generateBreakpointCommands() const
{
    QStringList breakpoint_commands ;
    for (BreakPoints::const_iterator itBreakpoint = _breakpoints.begin() ; itBreakpoint != _breakpoints.end() ; ++itBreakpoint )
    {
        if ( itBreakpoint.value().command.isEmpty() )
        {
            QFileInfo sourceInfo( itBreakpoint.value().file );
            QString module = sourceInfo.baseName();
            module = module.toLower();
            if (module.length() > 0)
            {
                module[0] = module[0].toUpper();
                QFile f ( itBreakpoint.value().file );
                if ( f.open( QFile::ReadOnly | QFile::Text ) )
                {
                    QString command = QString("break @ %1 %2 %3")
                        .arg(module)
                        .arg( QString::number( itBreakpoint.value().toLine  ) )
                        .arg( QString::number( itBreakpoint.value().toColumn ) );

                    breakpoint_commands << command;
                }
            }
        }
        else
            breakpoint_commands << itBreakpoint.value().command ;
    }

    breakpoint_commands.removeDuplicates();
    return breakpoint_commands;
}

void OCamlDebug::receiveDataFromProcessStdError()
{
    if (process_p)
    {
        process_p->setReadChannel(QProcess::StandardError);
        readChannel();
    }
}

void OCamlDebug::receiveDataFromProcessStdOutput()
{
    if (process_p)
    {
        process_p->setReadChannel(QProcess::StandardOutput);
        readChannel();
    }
}

void OCamlDebug::readChannel()
{
    QByteArray raw ;
    while (process_p->canReadLine())
    {
        QByteArray raw = process_p->readLine();
        appendText( raw );
    }
    raw = process_p->read(256);
    appendText( raw );
}

void OCamlDebug::appendText( const QByteArray &text )
{
    bool display = true;
    bool debugger_command = false;
    DebuggerCommand::Option command_option = DebuggerCommand::SHOW_ALL_OUTPUT ;
    QString command ;
    if ( !_command_queue.isEmpty() )
    {
        command_option = _command_queue.first().option();
        command = _command_queue.first().command();
    }
    QString data = QString::fromLatin1( text ).remove( '\r' );
    bool command_completed = readyRx.indexIn( data ) >= 0;
    if ( command_completed )
    {
        debugger_command = true;
        data =  data.remove( readyRx );
    }

    if ( deleteBreakpointRx.indexIn(data) == 0 )
    {
        QString _id = deleteBreakpointRx.cap(1);
        bool ok;
        int id = _id.toInt(&ok);
        if (ok)
        {
            debugger_command = true;
            _breakpoints.remove( id );
            emit breakPointList( _breakpoints );
        }
        saveBreakpoints();
    }
    else if ( hitBreakpointRx.indexIn(data) == 0 )
    {
        QList<int> ids ;
        int pos = 0;
        while ((pos = hitBreakpointIdRx.indexIn(data, pos)) != -1) 
        {
            QString sid = hitBreakpointIdRx.cap( 0 );
            bool ok;
            int id = sid.simplified().toInt( &ok );
            if ( ok )
                ids << id;
            pos += hitBreakpointIdRx.matchedLength();
        }
        debugger_command = true;
        _breakpoint_hits = ids;
    }
    else if ( newBreakpointRx.indexIn(data) == 0 )
    {
        BreakPoint breakpoint;
        breakpoint.command = command;
        QString _id = newBreakpointRx.cap(1);
        breakpoint.file = newBreakpointRx.cap(2);
        QString _line = newBreakpointRx.cap(3);
        QString _from_char = newBreakpointRx.cap(4);
        QString _to_char = newBreakpointRx.cap(5);
        bool ok;
        breakpoint.id = _id.toInt(&ok);
        if (ok)
            breakpoint.fromLine = _line.toInt(&ok);
        breakpoint.toLine = breakpoint.fromLine;
        if (ok)
            breakpoint.fromColumn = _from_char.toInt(&ok);
        if (ok)
            breakpoint.toColumn = _to_char.toInt(&ok);
        if (ok)
        {
            debugger_command = true;
            _breakpoints[ breakpoint.id ] = breakpoint;
            emit breakPointList( _breakpoints );
        }
        saveBreakpoints();
    }
    else if ( emacsLineInfoRx.indexIn(data) == 0 )
    {
        display = false ;
        QString file = emacsLineInfoRx.cap(1);
        QString start_char_str = emacsLineInfoRx.cap(2);
        QString end_char_str = emacsLineInfoRx.cap(3);
        QString instruction = emacsLineInfoRx.cap(4);
        bool after = instruction == "after";

        bool ok;
        int start_char = start_char_str.toInt(&ok);
        if (ok)
        {
            int end_char = end_char_str.toInt(&ok);

            if (ok)
            {
                emit stopDebugging( file , start_char , end_char , after );
                if ( _time >= 0)
                {
                    _time_info[blockCount()] = _time ;
                    updateDebugTimeAreaWidth( 0 );
                }
            }
        }
        emit breakPointHit( _breakpoint_hits );
        _breakpoint_hits.clear();
    }
    else if ( timeInfoRx.exactMatch(data) )
    {
        QString time = timeInfoRx.cap(1);
        bool ok;
        int t = time.toInt(&ok);
        if ( ok )
        {
            debugger_command = true;
            _time = t;
        }
    }
    else if ( ocamlrunConnectionRx.exactMatch(data) )
    {
        debugger_command = true;
        _ocamlrun_p->startApplication( _current_port );
    }
    else if ( emacsHaltInfoRx.exactMatch(data) )
    {
        display = false ;
        emit stopDebugging( QString() , 0 , 0 , false);
        if ( _time >= 0)
        {
            _time_info[blockCount()] = _time ;
            updateDebugTimeAreaWidth( 0 );
        }
        emit breakPointHit( _breakpoint_hits );
        _breakpoint_hits.clear();
    }
    else
    {
        for (QList<QRegExp>::iterator itRx = _debuggerOutputsRx.begin() ; itRx !=_debuggerOutputsRx.end() ; ++itRx )
        {
            if ( itRx->exactMatch(data) )
                debugger_command = true;
        }
    }

    if ( display )
    {
        if ( !_command_queue.isEmpty() )
            _command_queue.first().appendResult( data );

        if ( 
                 !debugger_command 
                 && 
                 command_option == DebuggerCommand::HIDE_DEBUGGER_OUTPUT
           )
        {
            if ( !data.isEmpty() )
                if ( !_command_queue.isEmpty() )
                    _command_queue.first().setOption( DebuggerCommand::HIDE_DEBUGGER_OUTPUT_SHOW_PROMT );
        }
        if ( _time >= 0 && command_completed )
            _time_info[blockCount()] = _time ;
        if ( 
                command_option == DebuggerCommand::SHOW_ALL_OUTPUT
                ||
                command_option == DebuggerCommand::IMMEDIATE_COMMAND
                || 
                ( 
                 !debugger_command 
                 && 
                 (
                  command_option == DebuggerCommand::HIDE_DEBUGGER_OUTPUT
                  ||
                  command_option == DebuggerCommand::HIDE_DEBUGGER_OUTPUT_SHOW_PROMT
                 )
                )
                ||
                ( 
                 command_completed
                 &&
                 command_option == DebuggerCommand::HIDE_DEBUGGER_OUTPUT_SHOW_PROMT
                )
           )
        {
            undisplayCommandLine();
            QTextCursor cur = textCursor();
            cur.movePosition(QTextCursor::End, QTextCursor::MoveAnchor) ;
            setTextCursor(cur);
            cur.insertText(text);
            displayCommandLine();
            updateDebugTimeAreaWidth( 0 );
        }
        debugTimeArea->repaint();
    }
    if ( command_completed )
    {
        if ( !_command_queue.isEmpty() )
        {
            QString command = _command_queue.first().command();
            QString response = _command_queue.first().result();
            emit debuggerCommand( command, response );
            _command_queue.removeFirst();
        }
        processOneQueuedCommand();
    }
}

void OCamlDebug::debuggerInterrupt()
{
#if defined (Q_OS_WIN32)
    QMessageBox::information (this, tr("Information"), 
            tr("Ctrl-C is not supported on Windows.") );
#else
    int pid = process_p->pid();
    ::kill( pid , SIGINT );
#endif
    _ocamlrun_p->debuggerInterrupt() ;
}

void OCamlDebug::debugger( const DebuggerCommand &command )
{
    if ( command.option() == DebuggerCommand::IMMEDIATE_COMMAND )
        _command_queue.clear();

    bool empty_queue = _command_queue.isEmpty() ;
    if ( _display_all_commands )
    {
        DebuggerCommand command_copy = command;
        command_copy.setOption( DebuggerCommand::SHOW_ALL_OUTPUT );
        _command_queue.append( command_copy );
    }
    else
        _command_queue.append( command );

    if ( empty_queue )
        processOneQueuedCommand();
}

void OCamlDebug::processOneQueuedCommand()
{
    if ( process_p == NULL )
        return ;
    while ( !_command_queue.isEmpty() )
    {
        QString command = _command_queue.first().command();
        if ( command.isEmpty() )
            continue;
        bool show_command =
            _command_queue.first().option() == DebuggerCommand::SHOW_ALL_OUTPUT
            ||
            _command_queue.first().option() == DebuggerCommand::IMMEDIATE_COMMAND
            ;
        if( show_command )
        {
            saveLRU( command );
            _command_line = command ;
            _cursor_position = command.length();;
            displayCommandLine();
            _command_line += '\n';
        }
        process_p->write( (command+'\n').toLatin1() );
        if( show_command )
        {
            displayCommandLine();
            _command_line.clear();
            _command_line_last.clear();
            _cursor_position=0;
        }
        return;
    }
    debugTimeArea->repaint();
}

void OCamlDebug::saveLRU(const QString &command)
{
    if (!command.isEmpty())
    {
        _lru.removeAll(command);
        _lru << command;
        Options::set_opt("OCAMLDEBUG_COMMANDS", _lru);
    }
}

void OCamlDebug::wheelEvent ( QWheelEvent * event )
{
    if ( process_p )
    {
        if ( 
                ( ! ( event->modifiers() & Qt::ShiftModifier) ) 
                &&
                ( event->modifiers() & Qt::ControlModifier)
           )
        {
            if (event->delta() > 0 )
                debugger( DebuggerCommand( "previous", DebuggerCommand::HIDE_DEBUGGER_OUTPUT ) );
            else if (event->delta() < 0 )
                debugger( DebuggerCommand( "next", DebuggerCommand::HIDE_DEBUGGER_OUTPUT ) );

            event->ignore();
        }
        else if ( 
                ( event->modifiers() & Qt::ShiftModifier)
                &&
                ( ! ( event->modifiers() & Qt::ControlModifier) )
                )
        {
            if (event->delta() > 0 )
                debugger( DebuggerCommand( "backstep", DebuggerCommand::HIDE_DEBUGGER_OUTPUT ) );
            else if (event->delta() < 0 )
                debugger( DebuggerCommand( "step", DebuggerCommand::HIDE_DEBUGGER_OUTPUT ) );

            event->ignore();
        }
        else if ( 
                ( event->modifiers() & Qt::ShiftModifier)
                &&
                ( event->modifiers() & Qt::ControlModifier) 
                )
        {
            if (event->delta() > 0 )
                debugger( DebuggerCommand( "up", DebuggerCommand::HIDE_DEBUGGER_OUTPUT ) );
            else if (event->delta() < 0 )
                debugger( DebuggerCommand( "down", DebuggerCommand::HIDE_DEBUGGER_OUTPUT ) );

            event->ignore();
        }
        else
            QPlainTextEdit::wheelEvent( event );
    }
    else
        QPlainTextEdit::wheelEvent( event );
}

void OCamlDebug::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    QAction *displayCommandAct = new QAction( tr( "&Display all commands" ) , this );
    displayCommandAct->setCheckable( true );
    displayCommandAct->setChecked( _display_all_commands );
    connect( displayCommandAct, SIGNAL( triggered(bool) ), this, SLOT( displayAllCommands(bool) ) );
    menu->addAction( displayCommandAct );
    menu->exec(event->globalPos());

    delete displayCommandAct;
    delete menu;
}

void OCamlDebug::resizeEvent( QResizeEvent *e )
{
    QPlainTextEdit::resizeEvent( e );

    QRect cr = contentsRect();
    debugTimeArea->setGeometry( QRect( cr.left(), cr.top(), debugTimeAreaWidth(), cr.height() ) );
}

void OCamlDebug::updateDebugTimeArea( const QRect &rect, int dy )
{
    if ( dy )
        debugTimeArea->scroll( 0, dy );
    else
        debugTimeArea->update( 0, rect.y(), debugTimeArea->width(), rect.height() );

    if ( rect.contains( viewport()->rect() ) )
        updateDebugTimeAreaWidth( 0 );
}


void OCamlDebug::updateDebugTimeAreaWidth( int /* newBlockCount */ )
{
    setViewportMargins( debugTimeAreaWidth(), 0, 0, 0 );
}

int OCamlDebug::debugTimeAreaWidth()
{
    int digits = 1;
    int max = 0;
    for (QMap<int,int>::const_iterator it = _time_info.begin(); it != _time_info.end() ; ++it)
        max = qMax( max, it.value() );

    while ( max >= 10 )
    {
        max /= 10;
        ++digits;
    }
    digits+=1;
    if (digits < 5)
        digits = 5;

    int space = 3 + fontMetrics().width( QLatin1Char( 'M' ) ) * digits;

    return space;
}

void OCamlDebug::debugTimeAreaPaintEvent( QPaintEvent *event )
{
    QPainter painter( debugTimeArea );
    painter.fillRect( event->rect(), Qt::white );
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = ( int ) blockBoundingGeometry( block ).translated( contentOffset() ).top();
    int bottom = top + ( int ) blockBoundingRect( block ).height();
    while ( block.isValid() && top <= event->rect().bottom() )
    {
        if ( block.isVisible() && bottom >= event->rect().top() )
        {
            painter.setPen( Qt::gray );
            if ( blockNumber == 0)
            {
                painter.drawText( 0, top, debugTimeArea->width(), fontMetrics().height(),
                        Qt::AlignCenter, tr( "Time" ) );
            }
            else
            {
                if ( !_command_queue.isEmpty() && blockNumber == blockCount()-1 )
                {
                    painter.setPen( Qt::blue );
                    painter.drawText( 0, top, debugTimeArea->width(), fontMetrics().height(),
                            Qt::AlignRight, tr( "<run>" ) );
                }
                else
                {
                    QMap<int,int>::const_iterator tm = _time_info.find( blockNumber+1 );
                    if ( tm != _time_info.end() )
                    {
                        {
                            int time = tm.value() ;
                            if ( time >= 0 )
                            {
                                QString time = QString::number( tm.value() ) + ":";
                                painter.drawText( 0, top, debugTimeArea->width(), fontMetrics().height(),
                                        Qt::AlignRight, time );
                            }
                        }
                    }
                }
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + ( int ) blockBoundingRect( block ).height();
        ++blockNumber;
    }
}

int OCamlDebug::findFreeServerPort( int port ) const
{
    int offset = 0;
    int free_port;
    do
    {
        offset++;
        free_port = port + offset;
        QTcpServer tcp_server;
        if ( tcp_server.listen( QHostAddress( "127.0.0.1" ), free_port ) )
        {
            tcp_server.close();
            return free_port;
        }
        if ( free_port > _port_max )
            free_port = _port_min ;
    }
    while ( port != free_port );

    return 0;
}

OCamlDebugTime::OCamlDebugTime( OCamlDebug *d ) : QWidget( d )
{
    debugger = d;
    QPalette palette;
    palette.setColor(backgroundRole(), Qt::white);
    setPalette(palette);
    setMouseTracking( true );
}

QSize OCamlDebugTime::sizeHint() const
{
    return QSize( debugger->debugTimeAreaWidth(), 0 );
}

void OCamlDebugTime::paintEvent( QPaintEvent *event )
{
    debugger->debugTimeAreaPaintEvent( event );
}

void OCamlDebugTime::mouseDoubleClickEvent ( QMouseEvent * event )
{
    QTextCursor cur = debugger->cursorForPosition( event->pos() );
    QMap<int,int>::const_iterator tm = debugger->timeInfo().find( cur.blockNumber()+1 );
    if ( tm != debugger->timeInfo().end() )
    {
        int time = tm.value();
        if ( time >= 0 )
            debugger->debugger( DebuggerCommand( "goto " + QString::number(time), DebuggerCommand::HIDE_DEBUGGER_OUTPUT )  );
    }
    QWidget::mouseMoveEvent( event );
}

bool OCamlDebugTime::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip)
    {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        QTextCursor cur = debugger->cursorForPosition( helpEvent->pos() );
        QMap<int,int>::const_iterator tm = debugger->timeInfo().find( cur.blockNumber()+1 );
        if ( tm != debugger->timeInfo().end() )
        {
            int time = tm.value();
            if ( time >= 0 )
                QToolTip::showText(helpEvent->globalPos(), tr( "Double-click to return to the execution time %1" ).arg( QString::number( time ) ) ) ;
        } 
        else
        {
            QToolTip::hideText();
            event->ignore();
        }

        return true;
    }
    return QWidget::event(event);
}

void OCamlDebug::displayAllCommands( bool b )
{
    _display_all_commands = b;
    Options::set_opt( "DISPLAY_ALL_OCAMLDEBUG_COMMANDS", b );
}
