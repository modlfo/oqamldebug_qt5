#include <QtGui>
#include <QtDebug>
#include <QTreeWidgetItem>
#include <QStringListModel>
#include "ocamlstack.h"
#include "options.h"


OCamlStack::OCamlStack( QWidget *parent_p ) : 
    QWidget(parent_p),
    stackRx( "^#([0-9]*)  *Pc : ([0-9]+) *([^ ]+)  *char  *([0-9]+) *\\n?$" )
{
    setObjectName(QString("OCamlStack"));
    _current_frame = -1;

    layout_p = new QVBoxLayout( );
    stack_p = new QTreeWidget() ;
    connect( stack_p, SIGNAL( itemClicked ( QTreeWidgetItem * , int ) ), this, SLOT( expressionClicked( QTreeWidgetItem * , int ) ) );
    layout_p->addWidget( stack_p );
    layout_p->setContentsMargins( 0,0,0,0 );
    setLayout( layout_p );

    QStringList headers ;
    headers << tr( "Frame" ) << tr( "PC" ) << tr( "Module" ) << tr("Char Position") ;
    stack_p->setRootIsDecorated(false);
    stack_p->setColumnCount( headers.count() );
    stack_p->setHeaderLabels( headers );
    clearData();

    setAttribute(Qt::WA_DeleteOnClose);
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    setFont(font);
}

OCamlStack::~OCamlStack()
{
    clearData();
    delete layout_p;
    delete stack_p;
}

void OCamlStack::clearData()
{
    stack_p->clear();
    _stack.clear();
}

void OCamlStack::closeEvent(QCloseEvent *event)
{
    event->accept();
}

void OCamlStack::stopDebugging( const QString &file, int start_char, int end_char, bool after ) 
{
    if ( 
            _file       != file
            ||
            _start_char != start_char
            ||
            _end_char   != end_char
            ||
            _after      != after
       )
    {
        _file          = file;
        _start_char    = start_char;
        _end_char      = end_char;
        _after         = after;
        updateStack();
    }
}

void OCamlStack::updateStack()
{
    emit debugger( DebuggerCommand( "backtrace", DebuggerCommand::HIDE_ALL_OUTPUT ) );
    emit debugger( DebuggerCommand( "frame", DebuggerCommand::HIDE_ALL_OUTPUT ) );
}

void  OCamlStack::debuggerCommand( const QString &cmd, const QString &result)
{
    if ( cmd == "backtrace" )
    {
        _stack.clear();
        QStringList results = result.split( '\n' );
        for ( QStringList::const_iterator itResult = results.begin() ; itResult != results.end() ; ++itResult )
        {
            if ( stackRx.exactMatch( *itResult ) )
            {
                bool ok;
                StackItem item;
                item.frame = stackRx.cap(1).toInt( &ok );
                if ( !ok )
                    continue;
                item.pc = stackRx.cap(2).toInt( &ok );
                if ( !ok )
                    continue;
                item.module = stackRx.cap(3);
                item.position = stackRx.cap(4).toInt( &ok );
                if ( !ok )
                    continue;

                _stack.append( item );
            }
        }
    }
    else if ( cmd.startsWith( "do" ) || cmd.startsWith ("u") || cmd.startsWith( "fr" ) )
    {
        QStringList results = result.split( '\n' );
        for ( QStringList::const_iterator itResult = results.begin() ; itResult != results.end() ; ++itResult )
        {
            if ( stackRx.exactMatch( *itResult ) )
            {
                bool ok;
                StackItem item;
                _current_frame = stackRx.cap(1).toInt( &ok );
                if ( !ok )
                    _current_frame = -1;
            }
        }
    }
    else
        return;

    stack_p->clear();
    for (Stack::const_iterator itStack = _stack.begin() ; itStack != _stack.end() ; ++itStack )
    {
        QStringList item ;
        item 
            << QString::number( itStack->frame ) 
            << QString::number( itStack->pc ) 
            << itStack->module
            << QString::number( itStack->position ) 
            ;
        QTreeWidgetItem *item_p = new QTreeWidgetItem( item );
        item_p->setFlags( Qt::ItemIsEnabled );
        QFont f = font();
        bool is_current_frame = _current_frame == itStack->frame ;
        for ( int i=0 ; i<4 ; i++ )
        {
            item_p->setFont( i, f );
            item_p->setToolTip( i, tr( "Click to go to the frame '%0'." ).arg( QString::number( itStack->frame ) ) );
            if ( is_current_frame )
                item_p->setBackgroundColor( i, Qt::yellow );
        }
        stack_p->header()->setResizeMode( 3, QHeaderView::ResizeToContents ); 

        stack_p->addTopLevelItem( item_p );
    }
}

void OCamlStack::expressionClicked( QTreeWidgetItem *item_p , int )
{
    if (item_p)
        emit debugger( DebuggerCommand( "frame "+item_p->text(0) , DebuggerCommand::HIDE_ALL_OUTPUT ) );
}

void OCamlStack::debuggerStarted(bool)
{
}
