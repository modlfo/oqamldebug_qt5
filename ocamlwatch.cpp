#include <QtGui>
#include <QtDebug>
#include <QTreeWidgetItem>
#include "ocamlwatch.h"
#include "options.h"


OCamlWatch::OCamlWatch( QWidget *parent_p, int i ) : QWidget(parent_p), id(i)
{
    setObjectName(QString("OCamlWatch%1").arg( QString::number(id) ));

    layout_p = new QVBoxLayout( );
    variables_p = new QTreeWidget() ;
    layout_p->addWidget( variables_p );
    layout_p->setContentsMargins( 0,0,0,0 );
    setLayout( layout_p );

    variables_p->setColumnCount( 2 );
    variables_p->setHeaderLabels( QStringList() << tr("Expresssion") << tr("Value") );
    clearData();

    setAttribute(Qt::WA_DeleteOnClose);
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    setFont(font);

    restoreWatches();
}

OCamlWatch::~OCamlWatch()
{
    clearData();
    delete layout_p;
    delete variables_p;
}

void OCamlWatch::clearData()
{
    variables_p->clear();
    _watches.clear();
}

void OCamlWatch::saveWatches()
{
    QStringList sav;
    for (QList<Watch>::const_iterator itWatch = _watches.begin() ; itWatch != _watches.end() ; ++itWatch )
    {
        sav << itWatch->variable ;
        if ( itWatch->display )
            sav << "1";
        else
            sav << "0";
    }
    Options::set_opt( objectName()+"_VARIABLES", sav );
}

void OCamlWatch::restoreWatches()
{
    QStringList vars = Options::get_opt_strlst( objectName()+"_VARIABLES" );
    for (QStringList::const_iterator itVar = vars.begin(); itVar != vars.end() ; ++itVar)
    {
        QString variable = *itVar;
        ++itVar;
        bool displayed = itVar->toInt() != 0;
        watch( variable, displayed );
    }
}

void OCamlWatch::closeEvent(QCloseEvent *event)
{
    event->accept();
}

void OCamlWatch::watch( const QString &variable, bool display ) 
{
    if ( variable.isEmpty() )
        return ;

    if ( variables().contains( variable ) )
        return ;

    Watch w;
    w.variable = variable;
    w.display = display;
    _watches.append( w );
    updateWatches();
    saveWatches();
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
    variables_p->clear();
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
            QString value  = result.trimmed() ;
            bool modified = value != itWatch->value ;
            itWatch->value = value ;
            QStringList item ;
            item << itWatch->variable << itWatch->value ;
            QTreeWidgetItem *item_p = new QTreeWidgetItem( item );
            if ( modified )
            {
                item_p->setBackground( 0, QBrush( Qt::yellow ) );
                item_p->setBackground( 1, QBrush( Qt::yellow ) );
            }
            variables_p->addTopLevelItem( item_p );
        }
    }
}

QStringList  OCamlWatch::variables() const
{
    QStringList ret;
    for (QList<Watch>::const_iterator itWatch = _watches.begin() ; itWatch != _watches.end() ; ++itWatch )
        ret << itWatch->variable ;
    return ret;
}
