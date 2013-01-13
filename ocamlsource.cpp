#include <QtGui>
#include "ocamlsource.h"
#include "options.h"
#include "ocamlsourcehighlighter.h"
#include <QTimer>
#include <QAction>
#include <QMenu>

const static int timer_values[] = { 50, 50, 50, 25, 25  } ;
const static int nb_timer_values = sizeof( timer_values ) / sizeof( int );


OCamlSource::OCamlSource()
{
    _from_user_loaded = true;
    lineSearchArea = new OCamlSourceSearch( this );
    lineSearchArea->hide();
    lineSearchArea->setEnabled(false);

    connect( lineSearchArea, SIGNAL( textChanged( const QString & ) ), this, SLOT( searchTextChanged( const QString & ) ) );
    connect( lineSearchArea, SIGNAL( returnPressed() ), this, SLOT( nextTextSearch() ) );

    lineNumberArea = new OCamlSourceLineNumberArea( this );
    connect( this, SIGNAL( blockCountChanged( int ) ), this, SLOT( updateLineNumberAreaWidth( int ) ) );
    connect( this, SIGNAL( updateRequest( QRect, int ) ), this, SLOT( updateLineNumberArea( QRect, int ) ) );

    updateLineNumberAreaWidth( 0 );

    setAttribute( Qt::WA_DeleteOnClose );
    highlighter = new OCamlSourceHighlighter( this->document() );
    _start_char = 0;
    _end_char = 0;
    _after = false;
    setReadOnly( true );
    QFont font( "Monospace" );
    font.setStyleHint( QFont::TypeWriter );
    setFont( font );
    timer_index = 0;
    markCurrentLocationTimer = new QTimer();
    connect ( markCurrentLocationTimer , SIGNAL( timeout() ), this , SLOT( markCurrentLocation() ) );
    markCurrentLocationTimer->setSingleShot( true );
    file_watch_p = NULL;
}

OCamlSource::~OCamlSource()
{
    delete markCurrentLocationTimer;
    if ( file_watch_p )
        delete file_watch_p;
}

int OCamlSource::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax( 1, blockCount() );
    while ( max >= 10 )
    {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().width( QLatin1Char( '9' ) ) * digits;

    return space;
}

void OCamlSource::updateLineNumberAreaWidth( int /* newBlockCount */ )
{
    setViewportMargins( lineNumberAreaWidth(), 0, 0, 0 );
}

void OCamlSource::searchTextChanged( const QString & )
{
    if ( !lineSearchArea->text().isEmpty() )
    {
        bool search_matched = toPlainText().contains( lineSearchArea->text() ) ;
        if ( search_matched )
            highlighter->searchWord( QRegExp( QRegExp::escape( lineSearchArea->text() ) ) );
        QPalette palette;
        if ( search_matched )
            palette.setColor(lineSearchArea->backgroundRole(), Qt::green );
        else
            palette.setColor(lineSearchArea->backgroundRole(), Qt::red );
        lineSearchArea->setPalette(palette); 
    }
}

void OCamlSource::nextTextSearch() 
{
    if ( !lineSearchArea->text().isEmpty() )
    {
        if ( toPlainText().contains( lineSearchArea->text() ) )
        {
            if ( !find( lineSearchArea->text(), QTextDocument::FindCaseSensitively ) )
            {
                QTextCursor cur = textCursor() ;
                cur.movePosition( QTextCursor::Start );
                setTextCursor( cur );
                find( lineSearchArea->text(), QTextDocument::FindCaseSensitively ) ;
            }
            centerCursor();
        }
    }
}

void OCamlSource::updateLineNumberArea( const QRect &rect, int dy )
{
    if ( dy )
        lineNumberArea->scroll( 0, dy );
    else
        lineNumberArea->update( 0, rect.y(), lineNumberArea->width(), rect.height() );

    if ( rect.contains( viewport()->rect() ) )
        updateLineNumberAreaWidth( 0 );
}

void OCamlSource::keyPressEvent ( QKeyEvent * e )
{
    switch (e->key())
    {
        case Qt::Key_Enter:
            if ( lineSearchArea->isEnabled() )
            {
                nextTextSearch();
            }
            break;
        case Qt::Key_Escape:
            if ( lineSearchArea->isEnabled() )
            {
                lineSearchArea->hide();
                lineSearchArea->setEnabled(false);
                highlighter->searchWord( QRegExp() );
            }
            else
                emit releaseFocus();
            break;

        default:
            {
                QString input_text = e->text() ;
                input_text = input_text.simplified();
                if ( !input_text.isEmpty() )
                {
                    lineSearchArea->show();
                    lineSearchArea->setEnabled(true);
                    lineSearchArea->setFocus();
                    lineSearchArea->setText( e->text() );
                }
            }
            break;
    }
    e->accept();
}

void OCamlSource::resizeEvent( QResizeEvent *e )
{
    QPlainTextEdit::resizeEvent( e );

    QRect cr = contentsRect();
    lineNumberArea->setGeometry( QRect( cr.left(), cr.top(), lineNumberAreaWidth(), cr.height() ) );
    resizeLineSearch();
}

void OCamlSource::resizeLineSearch()
{
    QRect cr = contentsRect();
    int max_w = cr.width()-lineNumberAreaWidth();
    int w = lineSearchArea->sizeHint().width();
    if ( w > max_w )
        w = max_w ;
    int h =lineSearchArea->sizeHint().height();
    int x = cr.right()-w ;
    int y = cr.bottom()-h;
    QScrollBar *vert_scrollbar_p = verticalScrollBar();
    if ( vert_scrollbar_p && vert_scrollbar_p->isVisible() )
        x -= vert_scrollbar_p->width();
    QScrollBar *hor_scrollbar_p = horizontalScrollBar();
    if ( hor_scrollbar_p && hor_scrollbar_p->isVisible())
        y -= hor_scrollbar_p->height();
    lineSearchArea->setGeometry( QRect( x, y , w, h ) );
}

void OCamlSource::lineNumberAreaPaintEvent( QPaintEvent *event )
{
    QPainter painter( lineNumberArea );
    painter.fillRect( event->rect(), Qt::lightGray );
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = ( int ) blockBoundingGeometry( block ).translated( contentOffset() ).top();
    int bottom = top + ( int ) blockBoundingRect( block ).height();
    while ( block.isValid() && top <= event->rect().bottom() )
    {
        if ( block.isVisible() && bottom >= event->rect().top() )
        {
            QString number = QString::number( blockNumber + 1 );
            painter.setPen( Qt::black );
            painter.drawText( 0, top, lineNumberArea->width(), fontMetrics().height(),
                              Qt::AlignRight, number );
        }

        block = block.next();
        top = bottom;
        bottom = top + ( int ) blockBoundingRect( block ).height();
        ++blockNumber;
    }
}

bool OCamlSource::loadFile( const QString &fileName )
{
    QFile file( fileName );
    _start_char = 0;
    _end_char = 0;
    _after = false;
    if ( !file.open( QFile::ReadOnly | QFile::Text ) )
        return false;

    QTextStream in( &file );
    QApplication::setOverrideCursor( Qt::WaitCursor );
    setPlainText( in.readAll() );
    QApplication::restoreOverrideCursor();
    file.close();

    setCurrentFile( fileName );
    return true;
}

QString OCamlSource::userFriendlyCurrentFile()
{
    QString name = strippedName( curFile );
    if (!_from_user_loaded)
        name = "[" + name + "]";
    return name;
}

void OCamlSource::closeEvent( QCloseEvent *event )
{
    event->accept();
}

void OCamlSource::setCurrentFile( const QString &fileName )
{
    curFile = QFileInfo( fileName ).canonicalFilePath();
    setWindowTitle( userFriendlyCurrentFile() );
    if ( file_watch_p )
        delete file_watch_p;
    file_watch_p = new FileSystemWatcher( fileName );
    connect ( file_watch_p , SIGNAL( fileChanged ( ) ) , this , SLOT( fileChanged () ) , Qt::QueuedConnection );
}

QString OCamlSource::strippedName( const QString &fullFileName )
{
    return QFileInfo( fullFileName ).fileName();
}

void OCamlSource::markBreakPoints(bool unmark)
{
    QFileInfo current_file_info( curFile ) ;
    BreakPoints::const_iterator itBreakpoint ;

    for( itBreakpoint = _breakpoints.begin(); itBreakpoint != _breakpoints.end() ; ++itBreakpoint )
    {
        QFileInfo breakpoint_file_info( itBreakpoint.value().file ) ;
        if ( breakpoint_file_info.fileName() == current_file_info.fileName() )
        {
            QTextCharFormat selectedFormat;

            if ( unmark )
                selectedFormat.setBackground( QColor( Qt::white ) );
            else
                selectedFormat.setBackground( QColor( Qt::red ).lighter() );

            QTextCursor cur = textCursor();
            int line        = itBreakpoint.value().fromLine;
            int from_column = itBreakpoint.value().fromColumn;
            int to_column   = itBreakpoint.value().toColumn;

            cur.movePosition( QTextCursor::Start, QTextCursor::MoveAnchor );
            cur.movePosition( QTextCursor::NextBlock, QTextCursor::MoveAnchor, line-1 );
            cur.movePosition( QTextCursor::NextCharacter, QTextCursor::MoveAnchor, from_column-1 );
            cur.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor, to_column - from_column );

            cur.mergeCharFormat( selectedFormat );
        }
    }

    if ( !unmark )
    {
        for( itBreakpoint = _breakpoints.begin(); itBreakpoint != _breakpoints.end() ; ++itBreakpoint )
        {
            QFileInfo breakpoint_file_info( itBreakpoint.value().file ) ;
            if ( breakpoint_file_info.fileName() == current_file_info.fileName() )
            {
                QTextCharFormat selectedFormat;

                selectedFormat.setBackground( QColor( Qt::red ) );

                QTextCursor cur = textCursor();
                int line        = itBreakpoint.value().fromLine;
                int from_column = itBreakpoint.value().fromColumn;
                int to_column   = itBreakpoint.value().toColumn;

                cur.movePosition( QTextCursor::Start, QTextCursor::MoveAnchor );
                cur.movePosition( QTextCursor::NextBlock, QTextCursor::MoveAnchor, line-1 );
                cur.movePosition( QTextCursor::NextCharacter, QTextCursor::MoveAnchor, from_column-1 );
                cur.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1 );
                cur.mergeCharFormat( selectedFormat );

                if ( to_column - from_column > 2 )
                {
                    cur.movePosition( QTextCursor::Start, QTextCursor::MoveAnchor );
                    cur.movePosition( QTextCursor::NextBlock, QTextCursor::MoveAnchor, line-1 );
                    cur.movePosition( QTextCursor::NextCharacter, QTextCursor::MoveAnchor, to_column - 2 );
                    cur.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1 );
                    cur.mergeCharFormat( selectedFormat );
                }
            }
        }
    }
}

void OCamlSource::markCurrentLocation()
{
    timer_index++;
    if ( _start_char != 0  && _end_char != 0 )
    {
        if ( timer_index < nb_timer_values )
        { // blink
            QTextCharFormat selectedFormat;
            QColor outlineColor;

            if ( timer_index % 2 == 0 )
                outlineColor = QColor( Qt::darkYellow );
            else
                outlineColor = QColor( Qt::yellow );

            selectedFormat.setBackground( outlineColor );
            QTextCursor cur = textCursor();
            cur.movePosition( QTextCursor::Start, QTextCursor::MoveAnchor );
            cur.movePosition( QTextCursor::NextCharacter, QTextCursor::MoveAnchor, _start_char );
            cur.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor, _end_char - _start_char );

            cur.mergeCharFormat( selectedFormat );
        }
        else
        {
           
            QColor currentPositionColor = QColor( Qt::darkYellow );
            QColor currentInstructionColor = QColor( Qt::yellow );
            QTextCharFormat currentPositionFormat;
            currentPositionFormat.setBackground( currentPositionColor );
            QTextCharFormat currentInstructionFormat;
            currentInstructionFormat.setBackground( currentInstructionColor );

            QTextCursor cur = textCursor();
            cur.movePosition( QTextCursor::Start, QTextCursor::MoveAnchor );
            cur.movePosition( QTextCursor::NextCharacter, QTextCursor::MoveAnchor, _start_char );
            cur.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor, _end_char - _start_char );

            cur.mergeCharFormat( currentInstructionFormat );

            int current_position ;
            if (_after)
                current_position = _end_char - 1;
            else
                current_position = _start_char;

            cur.movePosition( QTextCursor::Start, QTextCursor::MoveAnchor );
            cur.movePosition( QTextCursor::NextCharacter, QTextCursor::MoveAnchor, current_position );
            cur.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor );

            cur.mergeCharFormat( currentPositionFormat );
        }
    }
    if ( timer_index < nb_timer_values )
    {
        markCurrentLocationTimer->setInterval( timer_values[ timer_index % nb_timer_values ] );
        markCurrentLocationTimer->start();
    }
}

QString OCamlSource::stopDebugging( const QString &file, int start_char, int end_char , bool after)
{
    if ( curFile != file )
        loadFile( file );
    QTextCharFormat unselectedFormat;
    unselectedFormat.setBackground( Qt::white );

    QTextCursor cur = textCursor();
    cur.movePosition( QTextCursor::Start, QTextCursor::MoveAnchor );
    cur.movePosition( QTextCursor::End, QTextCursor::KeepAnchor );
    cur.mergeCharFormat( unselectedFormat );

    cur.movePosition( QTextCursor::Start, QTextCursor::MoveAnchor );
    cur.movePosition( QTextCursor::NextCharacter, QTextCursor::MoveAnchor, start_char );

    setTextCursor( cur );

    centerCursor();
    _start_char = start_char;
    _end_char = end_char;
    _after = after;
    timer_index = 0;
    markBreakPoints(false);
    markCurrentLocation();

    cur.movePosition( QTextCursor::Start, QTextCursor::MoveAnchor );
    cur.movePosition( QTextCursor::NextCharacter, QTextCursor::MoveAnchor, _start_char );
    cur.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor, _end_char - _start_char );
    QString text = cur.selectedText();

    return text;
}

void OCamlSource::fileChanged ( )
{
    const QString &fileName = curFile;
    QFile file( fileName );
    _start_char = 0;
    _end_char = 0;
    if ( file.open( QFile::ReadOnly | QFile::Text ) )
    {
        QTextStream in( &file );
        setPlainText( in.readAll() );
        QApplication::restoreOverrideCursor();
        file.close();
    }
    breakPointList( _breakpoints );
}

void OCamlSource::displayVar ( )
{
    emit displayVariable( _selected_text );
    QTextCursor current_cur = textCursor();
    current_cur.clearSelection();
    setTextCursor( current_cur );
}

void OCamlSource::printVar ( )
{
    emit printVariable( _selected_text );
    QTextCursor current_cur = textCursor();
    current_cur.clearSelection();
    setTextCursor( current_cur );
}

void OCamlSource::watchVar ( )
{
    emit watchVariable( _selected_text );
    QTextCursor current_cur = textCursor();
    current_cur.clearSelection();
    setTextCursor( current_cur );
}

void OCamlSource::newBreakpoint ( )
{
    QFileInfo sourceInfo( curFile );
    QString module = sourceInfo.baseName();
    if (module.length() > 0)
    {
        module[0] = module[0].toUpper();
        QString command = QString("break @ %1 %2 %3")
            .arg(module)
            .arg( QString::number( _breakpoint_line ) )
            .arg( QString::number( _breakpoint_column ) )
            ;
        emit debugger( DebuggerCommand( command, DebuggerCommand::HIDE_DEBUGGER_OUTPUT ) );
    }
}

void OCamlSource::contextMenuEvent( QContextMenuEvent *event )
{
    QTextCursor current_cur = textCursor();
    QTextCursor mouse_position = cursorForPosition( event->pos() );
    QTextCursor cur = mouse_position;
    if ( current_cur.hasSelection() )
        cur = current_cur;

    _breakpoint_line   = mouse_position.blockNumber() + 1;
    _breakpoint_column = mouse_position.position() - mouse_position.block().position() + 1;
    if ( ! cur.hasSelection() )
    {
        cur.select(QTextCursor::WordUnderCursor);
        setTextCursor(cur);
    }
    _selected_text = cur.selectedText();

    QAction *breakAct = new QAction( tr( "&Set Breakpoint at line %1 column %2" )
            .arg( QString::number(mouse_position.blockNumber()+1))
            .arg( QString::number(mouse_position.columnNumber()+1))
            , this );
    breakAct->setStatusTip( tr( "Set a breakpoint to the current location" ) );
    connect( breakAct, SIGNAL( triggered() ), this, SLOT( newBreakpoint() ) );

    QAction *displayAct = NULL;
    if ( cur.hasSelection() )
    {
        displayAct = new QAction( tr( "&Display '%1'" )
                .arg( _selected_text )
                , this );
        connect( displayAct, SIGNAL( triggered() ), this, SLOT( displayVar() ) );
    }

    QAction *printAct = NULL;
    if ( cur.hasSelection() )
    {
        printAct = new QAction( tr( "&Print '%1'" )
                .arg( _selected_text )
                , this );
        connect( printAct, SIGNAL( triggered() ), this, SLOT( printVar() ) );
    }
    QAction *watchAct = NULL;
    if ( cur.hasSelection() )
    {
        watchAct = new QAction( tr( "&Watch '%1'" )
                .arg( _selected_text )
                , this );
        connect( watchAct, SIGNAL( triggered() ), this, SLOT( watchVar() ) );
    }

    QMenu *menu = createStandardContextMenu();

    menu->addAction( breakAct );
    if (displayAct)
        menu->addAction( displayAct );
    if (printAct)
        menu->addAction( printAct );
    if (watchAct)
        menu->addAction( watchAct );
    menu->exec( event->globalPos() );

    delete breakAct;
    if (displayAct)
        delete displayAct;
    if (printAct)
        delete printAct;
    if (watchAct)
        delete watchAct;
    delete menu;
}


void OCamlSource::mousePressEvent ( QMouseEvent * e )
{
    QTextCursor current_cur = textCursor();
    QTextCursor cur = cursorForPosition( e->pos() );
    if ( current_cur.hasSelection() )
        cur = current_cur;

    _breakpoint_line   = cur.blockNumber() + 1;
    _breakpoint_column = cur.position() - cur.block().position() + 1;
    if ( ! cur.hasSelection() )
    {
        cur.select(QTextCursor::WordUnderCursor);
        setTextCursor(cur);
    }
    _selected_text = cur.selectedText();
    if ( e->button() == Qt::LeftButton )
    {
        lineSearchArea->hide();
        lineSearchArea->setEnabled(false);
        setFocus();
    }

    if ( e->button() == Qt::MidButton )
    {
        watchVar();
    }
    else
        QPlainTextEdit::mousePressEvent(e);
}

void OCamlSource::breakPointList( const BreakPoints &b )
{
    markBreakPoints(true);
    _breakpoints = b;
    markBreakPoints(false);
}


// Line area
OCamlSourceLineNumberArea::OCamlSourceLineNumberArea( OCamlSource *editor ) : QWidget( editor )
{
    codeEditor = editor;
}

QSize OCamlSourceLineNumberArea::sizeHint() const
{
    return QSize( codeEditor->lineNumberAreaWidth(), 0 );
}

void OCamlSourceLineNumberArea::paintEvent( QPaintEvent *event )
{
    codeEditor->lineNumberAreaPaintEvent( event );
}

// line search
OCamlSourceSearch::OCamlSourceSearch( OCamlSource *editor ) : QLineEdit( editor )
{
    codeEditor = editor;
    searched_pattern = Options::get_opt_strlst( "SEARCHED_PATTERN" ) ;
    completer_p = new QCompleter( searched_pattern );
    setCompleter( completer_p );
    connect( this, SIGNAL( returnPressed() ), this, SLOT( addToLRU() ) );
}

OCamlSourceSearch::~OCamlSourceSearch( )
{
    delete completer_p;
}

void OCamlSourceSearch::addToLRU()
{
    QString v = text();
    searched_pattern.removeAll( v );
    searched_pattern.prepend( v );
    QStringListModel *model = static_cast<QStringListModel*>( completer_p->model() );
    model->setStringList( searched_pattern );
    Options::set_opt( "SEARCHED_PATTERN", searched_pattern ) ;
}

