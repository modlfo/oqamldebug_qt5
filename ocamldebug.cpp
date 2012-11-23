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
    emacsHaltInfoRx("^\\x001A\\x001AH.*$"),
    timeInfoRx("^Time : ([0-9]+) - pc : ([0-9]+) - .*$")
{
    file_watch_p = NULL;
    debugTimeArea = new OCamlDebugTime( this );
    connect( this, SIGNAL( blockCountChanged( int ) ), this, SLOT( updateDebugTimeAreaWidth( int ) ) );
    connect( this, SIGNAL( updateRequest( QRect, int ) ), this, SLOT( updateDebugTimeArea( QRect, int ) ) );

    updateDebugTimeAreaWidth( 0 );
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
    QPlainTextEdit::clear();
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

void OCamlDebug::startProcess( const QString &program , const QStringList &arguments )
{
    clear();
    _time_info.clear();
    _time = -1 ;
    _command_queue.clear();
    _command_queue << DebuggerCommand( "", DebuggerCommand::IMMEDIATE_COMMAND ) ;
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

    debugger( DebuggerCommand( "goto 0", DebuggerCommand::HIDE_ALL_OUTPUT ) );
    restoreBreakpoints();
    emit debuggerStarted( true );
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
                debugger( DebuggerCommand( command, DebuggerCommand::HIDE_ALL_OUTPUT) );
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
    DebuggerCommand::Option command_option = DebuggerCommand::SHOW_ALL_OUTPUT ;
    if ( !_command_queue.isEmpty() )
        command_option = _command_queue.first().option();
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
                if ( _time >= 0)
                {
                    _time_info[blockCount()] = _time ;
                    updateDebugTimeAreaWidth( 0 );
                }
            }
        }
    }
    else if ( timeInfoRx.exactMatch(data) )
    {
        QString time = timeInfoRx.cap(1);
        bool ok;
        int t = time.toInt(&ok);
        if ( ok )
            _time = t;
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
    }

    if ( display )
    {
        if ( !_command_queue.isEmpty() )
            _command_queue.first().appendResult( data );

        if ( 
                command_option == DebuggerCommand::SHOW_ALL_OUTPUT
                ||
                command_option == DebuggerCommand::IMMEDIATE_COMMAND
           )
        {
            undisplayCommandLine();
            if ( _time >= 0)
                _time_info[blockCount()] = _time ;
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
            tr("Ctrl-C is not supported on Windows.") )
#else
    int pid = process_p->pid();
    ::kill( pid , SIGINT );
#endif
}

void OCamlDebug::debugger( const DebuggerCommand &command )
{
    bool empty_queue = _command_queue.isEmpty() ;

    if ( command.option() == DebuggerCommand::IMMEDIATE_COMMAND )
        _command_queue.prepend( command );
    else    
        _command_queue.append( command );

    if ( empty_queue || command.option() == DebuggerCommand::IMMEDIATE_COMMAND )
        processOneQueuedCommand();
}

void OCamlDebug::processOneQueuedCommand()
{
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
        process_p->write( (command+'\n').toAscii() );
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
    menu->exec(event->globalPos());
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
                QMap<int,int>::const_iterator tm = _time_info.find( blockNumber+1 );
                if ( tm != _time_info.end() )
                {
                    if ( !_command_queue.isEmpty() && blockNumber == blockCount()-1 )
                    {
                        painter.setPen( Qt::blue );
                        painter.drawText( 0, top, debugTimeArea->width(), fontMetrics().height(),
                                Qt::AlignRight, tr( "<run>" ) );
                    }
                    else
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

        block = block.next();
        top = bottom;
        bottom = top + ( int ) blockBoundingRect( block ).height();
        ++blockNumber;
    }
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

