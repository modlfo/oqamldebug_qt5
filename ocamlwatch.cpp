#include <QtGui>
#include <QtDebug>
#include "ocamlwatch.h"
#include "options.h"


OCamlWatch::OCamlWatch( QWidget *parent_p, int i ) : QTextEdit(parent_p), id(i)
{
    clearData();
    setReadOnly( true );
    setUndoRedoEnabled( false );
    setAttribute(Qt::WA_DeleteOnClose);
    highlighter = new OCamlSourceHighlighter(this->document());
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    setFont(font);
}

OCamlWatch::~OCamlWatch()
{
    clearData();
}

void OCamlWatch::clearData()
{
    clear();
    _watches.clear();
}

void OCamlWatch::closeEvent(QCloseEvent *event)
{
    event->accept();
}

void OCamlWatch::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    menu->exec(event->globalPos());
    delete menu;
}

void OCamlWatch::watch( const QString &variable, bool display ) 
{
    Watch w;
    w.variable = variable;
    w.display = display;
    _watches.append( w );
    updateWatches();
}

QString OCamlWatch::command( const Watch &watch ) const
{
    if ( watch.display )
        return "display " + watch.variable;
    else
        return "print " + watch.variable;
}

void OCamlWatch::stopDebugging( const QString &, int , int , bool) 
{
    updateWatches();
}

void OCamlWatch::updateWatches()
{
    clear();
    for (QList<Watch>::const_iterator itWatch = _watches.begin() ; itWatch != _watches.end() ; ++itWatch )
    {
       emit debugger( command( *itWatch ), false );
    }
}

void  OCamlWatch::debuggerCommand( const QString &cmd, const QString &result)
{
    for (QList<Watch>::Iterator itWatch = _watches.begin() ; itWatch != _watches.end() ; ++itWatch )
    {
        if ( command( *itWatch ) == cmd )
        {
            itWatch->value = result ;
            QString text;
            text += "<TABLE border=\"0\">" ;
            text += "<TR><TH>" ;
            text += Qt::escape( itWatch->variable ) ;
            text += "</TH><TD>" ;
            text += Qt::escape( itWatch->value ) ;
            text += "</TD></TR>" ;
            text += "</TABLE>" ;
            insertHtml( text );
        }
    }
}
