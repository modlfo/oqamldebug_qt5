#include <QtGui>
#include <QtDebug>
#include <QTextStream>
#include "ocamlrun.h"
#include "options.h"


OCamlRun::OCamlRun( QWidget *parent_p , const QString &ocamlrun, const QString & app, const QStringList &arguments ) : QPlainTextEdit(parent_p)
{
    setEnabled( false );
    setReadOnly( false );
    setUndoRedoEnabled( false );
    setAttribute(Qt::WA_DeleteOnClose);
    process_p = NULL;
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    setFont(font);
    setApplication( app, arguments );
    setOCamlRun( ocamlrun );

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

void OCamlRun::setOCamlRun( const QString &ocamlrun )
{
    _ocamlrun = ocamlrun ;
}

void OCamlRun::setApplication( const QString &app, const QStringList &arguments )
{
    _ocamlapp = app;
    _arguments = arguments ;
}

void OCamlRun::startApplication( int port )
{
    QStringList args ;
    args << _ocamlapp << _arguments ;
    terminate();
    startProcess ( port, _ocamlrun , args);
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
             appendText( text.toAscii() );
         }
     }
     else
         QPlainTextEdit::keyPressEvent ( e );
}

void OCamlRun::keyReleaseEvent ( QKeyEvent * e )
{
    QPlainTextEdit::keyReleaseEvent ( e );
}

void OCamlRun::startProcess( int port, const QString &program , const QStringList &arguments )
{
    clear();
    process_p =  new QProcess(this) ;
    process_p->setProcessChannelMode(QProcess::MergedChannels);
    connect ( process_p , SIGNAL( readyReadStandardOutput() ) , this , SLOT( receiveDataFromProcessStdOutput()) );
    connect ( process_p , SIGNAL( readyReadStandardError() ) , this , SLOT( receiveDataFromProcessStdError()) );
    QStringList env = QProcess::systemEnvironment();
    env << "CAML_DEBUG_SOCKET=localhost:" + QString::number( port ) ;
    process_p->setEnvironment(env);
    process_p->start( program , arguments );
    _outstream.setDevice( process_p );
    if ( ! process_p->waitForStarted())
    {
        QMessageBox::warning( this , tr( "Error Executing Command" ) , program );
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

void OCamlRun::appendText( const QByteArray &text )
{
    QTextCursor cur = textCursor();
    cur.movePosition(QTextCursor::End, QTextCursor::MoveAnchor) ;
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

