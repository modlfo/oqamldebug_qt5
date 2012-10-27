#include <QtGui>
#include "ocamlsource.h"
#include "ocamlsourcehighlighter.h"
#include <QTimer>
#include <QAction>
#include <QMenu>

const static int timer_values[] = { 50, 50, 50, 25, 25  } ;
const static int nb_timer_values = sizeof( timer_values ) / sizeof( int );


OCamlSource::OCamlSource()
{
    _from_user_loaded = true;
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

void OCamlSource::updateLineNumberArea( const QRect &rect, int dy )
{
    if ( dy )
        lineNumberArea->scroll( 0, dy );
    else
        lineNumberArea->update( 0, rect.y(), lineNumberArea->width(), rect.height() );

    if ( rect.contains( viewport()->rect() ) )
        updateLineNumberAreaWidth( 0 );
}

void OCamlSource::resizeEvent( QResizeEvent *e )
{
    QPlainTextEdit::resizeEvent( e );

    QRect cr = contentsRect();
    lineNumberArea->setGeometry( QRect( cr.left(), cr.top(), lineNumberAreaWidth(), cr.height() ) );
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
    for( BreakPoints::const_iterator itBreakpoint = _breakpoints.begin(); itBreakpoint != _breakpoints.end() ; ++itBreakpoint )
    {
        QString file = QFileInfo( itBreakpoint.value().file ).canonicalFilePath();
        if ( file == curFile )
        {
            QTextCharFormat selectedFormat;

            if ( unmark )
                selectedFormat.setBackground( QColor( Qt::white ) );
            else
                selectedFormat.setBackground( QColor( Qt::red ) );

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
}

void OCamlSource::displayVar ( )
{
    QString command = QString("display %1").arg(_selected_text);
    emit debugger( command );
    QTextCursor current_cur = textCursor();
    current_cur.clearSelection();
    setTextCursor( current_cur );
}

void OCamlSource::printVar ( )
{
    QString command = QString("print %1").arg(_selected_text);
    emit debugger( command );
    QTextCursor current_cur = textCursor();
    current_cur.clearSelection();
    setTextCursor( current_cur );
}

void OCamlSource::newBreakpoint ( )
{
    QFileInfo sourceInfo( curFile );
    QString module = sourceInfo.baseName();
    module = module.toLower();
    if (module.length() > 0)
    {
        module[0] = module[0].toUpper();
        QString command = QString("break @ %1 # %2")
            .arg(module)
            .arg( QString::number( _breapoint_position ) );
        emit debugger( command );
    }
}

void OCamlSource::contextMenuEvent( QContextMenuEvent *event )
{
    QTextCursor current_cur = textCursor();
    QTextCursor cur = cursorForPosition( event->pos() );
    if ( current_cur.hasSelection() )
        cur = current_cur;

    _breapoint_position = cur.position();
    if ( ! cur.hasSelection() )
    {
        cur.select(QTextCursor::WordUnderCursor);
        setTextCursor(cur);
    }
    _selected_text = cur.selectedText();

    QAction *breakAct = new QAction( tr( "&Set Breakpoint at line %1 column %2" )
            .arg( QString::number(cur.blockNumber()+1))
            .arg( QString::number(cur.columnNumber()+1))
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

    QMenu *menu = createStandardContextMenu();

    menu->addAction( breakAct );
    if (displayAct)
        menu->addAction( displayAct );
    if (printAct)
        menu->addAction( printAct );
    menu->exec( event->globalPos() );

    delete breakAct;
    if (displayAct)
        delete displayAct;
    if (printAct)
        delete printAct;
}


void OCamlSource::mousePressEvent ( QMouseEvent * e )
{
    if ( e->button() == Qt::MidButton )
    {
        QTextCursor current_cur = textCursor();
        QTextCursor cur = cursorForPosition( e->pos() );
        if ( current_cur.hasSelection() )
            cur = current_cur;

        _breapoint_position = cur.position();
        if ( ! cur.hasSelection() )
        {
            cur.select(QTextCursor::WordUnderCursor);
            setTextCursor(cur);
        }
        _selected_text = cur.selectedText();
        printVar();
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
