#include <QtGui>
#include <QtDebug>
#include "ocamldebughighlighter.h"
#include "ocamldebug.h"
#include "options.h"
#if !defined(Q_OS_WIN32)
#include <sys/types.h>
#include <signal.h>
#endif


OCamlDebug::OCamlDebug( QWidget *parent_p , const QString &ocamldebug, const QStringList &arguments ) : QPlainTextEdit(parent_p),
    emacsLineInfoRx("^\\x001A\\x001AM([^:]*):([^:]*):([^:]*):([^:\\n]*)\\n*$") ,
    readyRx("^\\(ocd\\) *") ,
    deleteBreakpointRx("^Removed breakpoint ([0-9]+) at [0-9]+ : .*$"),
    newBreakpointRx("^Breakpoint ([0-9]+) at [0-9]+ : file ([^,]*), line ([0-9]+), characters ([0-9]+)-([0-9]+).*$"),
    emacsHaltInfoRx("^\\x001A\\x001AH.*$")
{
    file_watch_p = NULL;
    emit debuggerStarted( false );
    _lru = Options::get_opt_strlst ("OCAMLDEBUG_COMMANDS");
    setReadOnly( false );
    setUndoRedoEnabled( false );
    setAttribute(Qt::WA_DeleteOnClose);
    highlighter = new OCamlDebugHighlighter(this->document());
    process_p = NULL;
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    setFont(font);
    setArguments( arguments );
    setOCamlDebug( ocamldebug );

    setCursorWidth(3);
}

OCamlDebug::~OCamlDebug()
{
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
        if (process_p->waitForFinished( 10000 ) )
            process_p->kill();
        process_p->close();
        delete process_p;
        process_p = NULL;
        emit debuggerStarted( false );
    }
}

void OCamlDebug::setOCamlDebug( const QString &ocamldebug )
{
    _ocamldebug = ocamldebug ;
}

void OCamlDebug::setArguments( const QStringList &arguments )
{
    _ocamlapp.clear();
    foreach ( QString a , arguments )
    {
        QFileInfo arg_info(a);
        if ( (!arg_info.isDir())
                && arg_info.exists()
                && arg_info.isExecutable()
           )
        {
            _ocamlapp = arg_info.absoluteFilePath();
            if ( file_watch_p )
                delete file_watch_p;
            file_watch_p = new FileSystemWatcher( _ocamlapp );
            connect ( file_watch_p , SIGNAL( fileChanged ( ) ) , this , SLOT( fileChanged () ) , Qt::QueuedConnection );
            break;
        }
    }
    _arguments.clear();
    _arguments << "-emacs" << arguments ;
}

void OCamlDebug::fileChanged ( )
{
    QTextCursor cur = textCursor();
    cur.movePosition(QTextCursor::End, QTextCursor::MoveAnchor) ;
    cur.insertText("\n"+tr("Application %1 is modified.").arg(_ocamlapp)+"\n");
    stopDebug();
    startDebug();
}

void OCamlDebug::startDebug()
{
    startProcess (_ocamldebug , _arguments);
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
                     saveLRU( _command_line );
                     _command_line += '\n';
                     process_p->write( _command_line.toAscii() );
                     displayCommandLine();
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

void OCamlDebug::startProcess( const QString &program , const QStringList &arguments )
{
    clear();
    _command_queue.clear();
    _command_queue << "" ;
    process_p =  new QProcess(this) ;
    process_p->setProcessChannelMode(QProcess::MergedChannels);
    connect ( process_p , SIGNAL( readyReadStandardOutput() ) , this , SLOT( receiveDataFromProcessStdOutput()) );
    connect ( process_p , SIGNAL( readyReadStandardError() ) , this , SLOT( receiveDataFromProcessStdError()) );
    process_p->start( program , arguments );
    if ( ! process_p->waitForStarted())
    {
        QMessageBox::warning( this , tr( "Error Executing Command" ) , program );
        clear();
        return;
    }

    emit debuggerStarted( true );
    restoreBreakpoints();
}

void OCamlDebug::restoreBreakpoints()
{
    BreakPoints breakpoints = _breakpoints;
    _breakpoints.clear();
    emit breakPointList( _breakpoints );

    for (BreakPoints::const_iterator itBreakpoint = breakpoints.begin() ; itBreakpoint != breakpoints.end() ; ++itBreakpoint )
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
                int pos = -1;
                QString src = f.readAll();
                for ( int l=1 ; l < itBreakpoint.value().fromLine ; l++ )
                {
                   pos = src.indexOf( "\n", pos+1 ); 
                   if ( pos < 0 )
                       continue;
                }
                pos += itBreakpoint.value().fromColumn;

                QString command = QString("break @ %1 # %2")
                    .arg(module)
                    .arg( QString::number( pos ) );
                debugger( command );
            }
        }
    }
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
    QString data = QString::fromAscii( text );
    bool command_completed = readyRx.indexIn( data ) >= 0;
    data =  data.remove( readyRx );
    if ( deleteBreakpointRx.indexIn(data) == 0 )
    {
        QString _id = deleteBreakpointRx.cap(1);
        bool ok;
        int id = _id.toInt(&ok);
        if (ok)
        {
            _breakpoints.remove( id );
            emit breakPointList( _breakpoints );
        }
    }
    else if ( newBreakpointRx.indexIn(data) == 0 )
    {
        BreakPoint breakpoint;
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
            _breakpoints[ breakpoint.id ] = breakpoint;
            emit breakPointList( _breakpoints );
        }
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
            }
        }
    }
    else if ( emacsHaltInfoRx.exactMatch(data) )
    {
        display = false ;
        emit stopDebugging( QString() , 0 , 0 , false);
    }

    if ( display )
    {
        undisplayCommandLine();
        QTextCursor cur = textCursor();
        cur.movePosition(QTextCursor::End, QTextCursor::MoveAnchor) ;
        setTextCursor(cur);
        cur.insertText(text);
        displayCommandLine();
    }
    if ( command_completed )
    {
        if ( !_command_queue.isEmpty() )
            _command_queue.removeFirst();
        processOneQueuedCommand();
    }
}

void OCamlDebug::debuggerInterrupt()
{
#if defined (Q_OS_WIN32)
    QMessageBox::information (this, tr("Information"), 
            tr("Ctrl-C is not supported on Windows.") )
#else
    int pid = process_p->pid();
    ::kill( pid , SIGINT );
#endif
}

void OCamlDebug::debugger( const QString & command)
{
    if ( _command_queue.isEmpty() )
    {
        _command_queue.append( command );
        processOneQueuedCommand();
    }
    else
        _command_queue.append( command );
}

void OCamlDebug::processOneQueuedCommand()
{
    while ( !_command_queue.isEmpty() )
    {
        QString command = _command_queue.first();
        if ( command.isEmpty() )
            continue;
        saveLRU( command );
        _command_line = command ;
        _cursor_position = command.length();;
        displayCommandLine();
        _command_line += '\n';
        process_p->write( _command_line.toAscii() );
        displayCommandLine();
        _command_line.clear();
        _command_line_last.clear();
        _cursor_position=0;
        return;
    }
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
                debugger( "previous" );
            else if (event->delta() < 0 )
                debugger( "next" );

            event->ignore();
        }
        else if ( 
                ( event->modifiers() & Qt::ShiftModifier)
                &&
                ( ! ( event->modifiers() & Qt::ControlModifier) )
                )
        {
            if (event->delta() > 0 )
                debugger( "backstep" );
            else if (event->delta() < 0 )
                debugger( "step" );

            event->ignore();
        }
        else if ( 
                ( event->modifiers() & Qt::ShiftModifier)
                &&
                ( event->modifiers() & Qt::ControlModifier) 
                )
        {
            if (event->delta() > 0 )
                debugger( "up" );
            else if (event->delta() < 0 )
                debugger( "down" );

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
    if ( process_p )
        setReadOnly(true);
    QMenu *menu = createStandardContextMenu();
    menu->exec(event->globalPos());
    setReadOnly(false);
    delete menu;
}

