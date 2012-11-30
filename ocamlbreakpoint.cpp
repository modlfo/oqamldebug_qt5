#include <QtGui>
#include <QtDebug>
#include <QTreeWidgetItem>
#include <QStringListModel>
#include "ocamlbreakpoint.h"
#include "options.h"


OCamlBreakpoint::OCamlBreakpoint( QWidget *parent_p ) : 
    QWidget(parent_p)
{
    setObjectName( "OCamlBreakpoint" );

    layout_p = new QVBoxLayout( );
    breakpoints_p = new QTreeWidget() ;
    connect( breakpoints_p, SIGNAL( itemClicked ( QTreeWidgetItem * , int ) ), this, SLOT( expressionClicked( QTreeWidgetItem * , int ) ) );
    layout_p->addWidget( breakpoints_p );
    layout_p->setContentsMargins( 0,0,0,0 );
    setLayout( layout_p );

    QStringList headers ;
    headers << "" << tr("Id") << tr("From") << tr("To") << tr("File") ;
    breakpoints_p->setRootIsDecorated(false);
    breakpoints_p->setColumnCount( headers.count() );
    breakpoints_p->setHeaderLabels( headers );
    clearData();

    setAttribute(Qt::WA_DeleteOnClose);
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    setFont(font);
}

OCamlBreakpoint::~OCamlBreakpoint()
{
    clearData();
    delete layout_p;
    delete breakpoints_p;
}

void OCamlBreakpoint::clearData()
{
    breakpoints_p->clear();
    _breakpoint_hit.clear();
    _breakpoints.clear();
}

void OCamlBreakpoint::closeEvent(QCloseEvent *event)
{
    event->accept();
}

void OCamlBreakpoint::stopDebugging( const QString &, int , int , bool) 
{
    updateBreakpoints();
}

void  OCamlBreakpoint::updateBreakpoints()
{
    breakpoints_p->clear();
    for (BreakPoints::Iterator itBreakpoint = _breakpoints.begin() ; itBreakpoint != _breakpoints.end() ; ++itBreakpoint )
    {
        QStringList item ;
        item 
            << "" 
            << QString::number( itBreakpoint->id )
            << QString::number( itBreakpoint->fromLine ) + ":" + QString::number( itBreakpoint->fromColumn ) 
            << QString::number( itBreakpoint->toLine ) + ":" + QString::number( itBreakpoint->toColumn ) 
            << itBreakpoint->file.simplified()
            ;
        QTreeWidgetItem *item_p = new QTreeWidgetItem( item );
        bool hit = _breakpoint_hit.contains( itBreakpoint->id );
        if ( hit )
            for ( int i=1 ; i<5 ; i++ )
                item_p->setBackgroundColor( i, QColor( Qt::red ) );
        item_p->setFlags( Qt::ItemIsEnabled );
        QIcon delete_icon = QIcon( ":/images/delete.png" );
        item_p->setIcon( 0, delete_icon );
        item_p->setToolTip( 0, tr( "Click to delete this breakpoint." ) );
        item_p->setSizeHint( 0, delete_icon.availableSizes().at(0) );
        breakpoints_p->header()->resizeSection( 0, delete_icon.availableSizes().at(0).width() ); 
        breakpoints_p->header()->setResizeMode( 0, QHeaderView::Fixed ); 
        breakpoints_p->header()->setResizeMode( 3, QHeaderView::ResizeToContents ); 

        breakpoints_p->addTopLevelItem( item_p );
    }
}

void OCamlBreakpoint::expressionClicked( QTreeWidgetItem *item_p , int column )
{
    if (item_p && column==0)
    {
        bool ok;
        QString sid = item_p->text(1);
        int id = sid.toInt( &ok );
        if ( ok )
            emit debugger( DebuggerCommand( "del " + QString::number(id), DebuggerCommand::HIDE_ALL_OUTPUT ) );
    }
}

void OCamlBreakpoint::debuggerStarted( bool )
{
}

void  OCamlBreakpoint::breakPointList( const BreakPoints &b )
{
    _breakpoints = b;
    updateBreakpoints();
}

void  OCamlBreakpoint::breakPointHit( const QList<int> &h )
{
    _breakpoint_hit = h;
    updateBreakpoints();
}

