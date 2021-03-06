#include <QtGui>
#include <QtDebug>
#include <QTextStream>
#include "ocamlrun.h"
#include "options.h"
#include <QMessageBox>
#include <QAction>
#include <QMenu>

OCamlRun::OCamlRun( QWidget *parent_p , const Arguments & arguments ) : QPlainTextEdit(parent_p),
    _arguments( arguments )
{
    _verbose = Options::get_opt_bool( "OCAMLRUN_VERBOSE", false );
    setEnabled( false );
    setReadOnly( false );
    setUndoRedoEnabled( false );
    setAttribute(Qt::WA_DeleteOnClose);
    process_p = NULL;
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    setFont(font);
    setArguments( _arguments );

    setCursorWidth(3);
}

OCamlRun::~OCamlRun()
{
    terminate();
    clear();
}

void OCamlRun::clear()
{
    _cursor_position=0;
    QPlainTextEdit::clear();
}

void OCamlRun::terminate()
{
    if ( process_p )
    {
        process_p->terminate();
        if (process_p->waitForFinished( 1000 ) )
            process_p->kill();
        process_p->close();
        delete process_p;
        process_p = NULL;
    }
}

void OCamlRun::setArguments( const Arguments & arguments )
{
    _arguments = arguments ;
}

void OCamlRun::startApplication( int port )
{
    terminate();
    startProcess ( port );
}

void OCamlRun::closeEvent(QCloseEvent *event)
{
    event->accept();
}

void OCamlRun::keyPressEvent ( QKeyEvent * e )
{
     if (process_p)
     {
         QString text = e->text();
         if ( text.isEmpty() )
                 QPlainTextEdit::keyPressEvent ( e );
         else
         {
             switch (e->key())
             {
                 case Qt::Key_Return: 
                     _outstream << '\n';
                     break;
                 default:
                     _outstream << text ;
                     break;
             }
             _outstream.flush();
             appendText( text.toLatin1(), Qt::blue );
         }
     }
     else
         QPlainTextEdit::keyPressEvent ( e );
}

void OCamlRun::keyReleaseEvent ( QKeyEvent * e )
{
    QPlainTextEdit::keyReleaseEvent ( e );
}

void OCamlRun::startProcess( int port )
{
    clear();
    process_p =  new QProcess(this) ;
    process_p->setProcessChannelMode(QProcess::MergedChannels);
    connect ( process_p , SIGNAL( readyReadStandardOutput() ) , this , SLOT( receiveDataFromProcessStdOutput()) );
    connect ( process_p , SIGNAL( readyReadStandardError() ) , this , SLOT( receiveDataFromProcessStdError()) );
    QStringList env = QProcess::systemEnvironment();
    env << "CAML_DEBUG_SOCKET=127.0.0.1:" + QString::number( port ) ;
    process_p->setEnvironment(env);

    if ( _verbose )
        appendText( tr( "Executing %1 %2 ...\n" ).arg( _arguments.ocamlApp() ).arg( _arguments.ocamlAppArguments().join( " " ) ), Qt::gray );
    process_p->start( _arguments.ocamlApp() , _arguments.ocamlAppArguments() );
    _outstream.setDevice( process_p );
    if ( ! process_p->waitForStarted())
    {
        QMessageBox::warning( this , tr( "Error Executing Command" ) , _arguments.ocamlApp() );
        clear();
        return;
    }
}

void OCamlRun::receiveDataFromProcessStdError()
{
    if (process_p)
    {
        process_p->setReadChannel(QProcess::StandardError);
        readChannel();
    }
}

void OCamlRun::receiveDataFromProcessStdOutput()
{
    if (process_p)
    {
        process_p->setReadChannel(QProcess::StandardOutput);
        readChannel();
    }
}

void OCamlRun::readChannel()
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

void OCamlRun::appendText( const QByteArray &text, const QColor &color )
{
    QTextCursor cur = textCursor();
    QTextCharFormat format;
    format.setForeground( color );
    cur.movePosition(QTextCursor::End, QTextCursor::MoveAnchor) ;
    cur.setCharFormat( format );
    cur.insertText(text);
    cur.movePosition(QTextCursor::End, QTextCursor::MoveAnchor );
    setTextCursor(cur);
    ensureCursorVisible();
}


void OCamlRun::debuggerStarted( bool b )
{
    setEnabled( b );
    if ( !b )
    {
        clear();
        terminate();
    }
}

void OCamlRun::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();

    QAction *clearAct = new QAction( tr( "&Clear" ) , this );
    connect( clearAct, SIGNAL( triggered() ), this, SLOT( clear() ) );

    QAction *verboseAct = new QAction( tr( "&Verbose" ) , this );
    verboseAct->setCheckable( true );
    verboseAct->setChecked( _verbose );
    connect( verboseAct, SIGNAL( triggered(bool) ), this, SLOT( setVerbose(bool) ) );

    menu->addAction( verboseAct );
    menu->addAction( clearAct );
    menu->exec(event->globalPos());

    delete verboseAct;
    delete clearAct;
    delete menu;
}

void OCamlRun::setVerbose( bool b )
{
    _verbose = b;
    Options::set_opt( "OCAMLRUN_VERBOSE", b );
}

void OCamlRun::debuggerInterrupt()
{
}
