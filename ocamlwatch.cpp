#include <QtGui>
#include <QtDebug>
#include <QTreeWidgetItem>
#include <QStringListModel>
#include "ocamlwatch.h"
#include "textdiff.h"
#include "options.h"
#include <QHeaderView>
#include <QLineEdit>

OCamlWatch::OCamlWatch( QWidget *parent_p, int i ) : 
    QWidget(parent_p),
    id(i),
    variableRx( "^[^:]* *: ([^=]*[^= ]) *= *([^ ].*)$" )
{
    setObjectName(QString("OCamlWatch%1").arg( QString::number(id) ));

    layout_add_value_p = new QHBoxLayout( );
    add_value_label_p = new QLabel(tr("Add expression:"));
    add_value_p = new QLineEdit();
    add_values = Options::get_opt_strlst( "WATCHED_VARIABLES" ) ;
    add_value_completer_p = new QCompleter( add_values );
    add_value_p->setCompleter( add_value_completer_p );
    connect( add_value_p, SIGNAL( returnPressed() ), this, SLOT( addNewValue() ) );
    layout_add_value_p->addWidget( add_value_label_p );
    layout_add_value_p->addWidget( add_value_p );
    layout_p = new QVBoxLayout( );
    variables_p = new QTreeWidget() ;
    connect( variables_p, SIGNAL( itemClicked ( QTreeWidgetItem * , int ) ), this, SLOT( expressionClicked( QTreeWidgetItem * , int ) ) );
    layout_p->addWidget( variables_p );
    layout_p->addLayout( layout_add_value_p );
    layout_p->setContentsMargins( 0,0,0,0 );
    setLayout( layout_p );

    connect( variables_p->header(), SIGNAL( sectionResized ( int , int , int ) ), this, SLOT( columnResized( int, int, int) ) );

    QStringList headers ;
    headers << tr("Del") << tr("Expresssion") << tr("Type") << tr("Value") ;
    variables_p->setRootIsDecorated(false);
    variables_p->setColumnCount( headers.count() );
    variables_p->setHeaderLabels( headers );
    clearData();

    setAttribute(Qt::WA_DeleteOnClose);

    variables_p->header()->setSectionResizeMode( 0, QHeaderView::ResizeToContents );
    variables_p->header()->restoreState( Options::get_opt_array( QString("OCamlWatch%1_State").arg( QString::number(id) ) ) );
    variables_p->setSortingEnabled( true );
    restoreWatches();
}

OCamlWatch::~OCamlWatch()
{
    Options::set_opt( QString("OCamlWatch%1_State").arg( QString::number(id) ), variables_p->header()->saveState() );
    clearData();
    delete add_value_completer_p;
    delete add_value_p;
    delete add_value_label_p;
    delete layout_add_value_p;
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
    w.uptodate = false;
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
    for (QList<Watch>::Iterator itWatch = _watches.begin() ; itWatch != _watches.end() ; ++itWatch )
    {
        itWatch->uptodate = false;
        emit debugger( DebuggerCommand( command( *itWatch ), DebuggerCommand::HIDE_ALL_OUTPUT ) );
    }
}

bool OCamlWatch::displayDiff( const QString & str1, const QString & str2 ) const
{
    if ( str1 == str2 )
        return false ;

    QStringList patterns;
    patterns 
        << "^$"
        << "^Unbound identifier .*"
        ;
    foreach (const QString &pattern, patterns) 
    {
        QRegExp patternRx( pattern );
        if ( patternRx.exactMatch( str1 ) || patternRx.exactMatch( str2 ) )
            return false;
    }
    return true;
}

void  OCamlWatch::debuggerCommand( const QString &cmd, const QString &result)
{
    for (QList<Watch>::Iterator itWatch = _watches.begin() ; itWatch != _watches.end() ; ++itWatch )
    {
        if ( command( *itWatch ) == cmd && !itWatch->uptodate )
        {
            itWatch->uptodate = true;
            QString value  = result.trimmed() ;
            bool modified = false;
            if ( !itWatch->all_output.isEmpty() )
                modified = value != itWatch->all_output ;
            itWatch->all_output = value ;
            QStringList item ;
            QString type;
            if ( variableRx.exactMatch( itWatch->all_output ) )
            {
                type = variableRx.cap(1).trimmed();
                value = variableRx.cap(2).trimmed();
            }
            item << "" << itWatch->variable << type ;
            QTreeWidgetItem *item_p = new QTreeWidgetItem( item );
            item_p->setFlags( Qt::ItemIsEnabled );
            QFont f = font();
            f.setBold( modified );
            for ( int i=1 ; i<3 ; i++ )
            {
                if ( i!=3 )
                    item_p->setFont( i, f );
                if ( itWatch->display )
                    item_p->setToolTip( i, tr( "Click to print all sub-values of '%0'." ).arg( itWatch->variable ) );
                else
                    item_p->setToolTip( i, tr( "Click to hide all sub-values of '%0'." ).arg( itWatch->variable ) );
                item_p->setTextAlignment( i, Qt::AlignLeft | Qt::ElideRight | Qt::AlignVCenter );
            }
            QString printed_value ;
            if ( displayDiff( value, itWatch->value_only ) )
                printed_value = htmlDiff( value, itWatch->value_only );
            else
            {
                if ( modified )
                    printed_value = "<HTML><BODY><B>" + value.toHtmlEscaped() + "</B></BODY></HTML>";
                else
                    printed_value = "<HTML><BODY>" + value.toHtmlEscaped() + "</BODY></HTML>";
            }

            QLabel *value_p = new QLabel( printed_value );
            value_p->setWordWrap(true) ;
                
            int width = variables_p->header()->sectionSize( 3 ) ;
            item_p->setSizeHint( 3, QSize( width, value_p->heightForWidth( width )) );
            QIcon delete_icon = QIcon( ":/images/delete.png" );
            item_p->setIcon( 0, delete_icon );
            item_p->setToolTip( 0, tr( "Click to unwatch this variable." ) );
            item_p->setSizeHint( 0, delete_icon.availableSizes().at(0) );
            variables_p->header()->resizeSection( 0, delete_icon.availableSizes().at(0).width() ); 
            variables_p->header()->setSectionResizeMode( 0, QHeaderView::Fixed ); 
            variables_p->header()->setSectionResizeMode( 3, QHeaderView::ResizeToContents ); 

            variables_p->addTopLevelItem( item_p );
            variables_p->setItemWidget( item_p, 3, value_p );
            itWatch->value_only = value ;
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

void OCamlWatch::columnResized( int logical_index, int /*old_size*/, int new_size )
{
    if ( logical_index == 3 )
    {
        QTreeWidgetItem *item_p = NULL; 
        for ( int idx = 0; ( item_p = variables_p->topLevelItem( idx ) ) ; idx++ )
        {
            QLabel *label_p = dynamic_cast<QLabel*>(variables_p->itemWidget( item_p, 3 ));
            if ( label_p )
                item_p->setSizeHint( 3, QSize( new_size, label_p->heightForWidth( new_size )) );
        }
    }
}

void OCamlWatch::addNewValue()
{
    QString v = add_value_p->text();
    add_values.removeAll( v );
    add_values.prepend( v );
    QStringListModel *model = static_cast<QStringListModel*>( add_value_completer_p->model() );
    model->setStringList( add_values );
    Options::set_opt( "WATCHED_VARIABLES", add_values ) ;
    watch( v, true );
}

void OCamlWatch::expressionClicked( QTreeWidgetItem *item_p , int column )
{
    if (item_p)
    {
        for (QList<Watch>::Iterator itWatch = _watches.begin() ; itWatch != _watches.end() ; ++itWatch )
        {
            if ( item_p->text(1) == itWatch->variable )
            {
                if ( column == 0 )
                    _watches.erase(itWatch);
                else
                    itWatch->display = !itWatch->display ;
                break;
            }
        }
        saveWatches();
        updateWatches();
    }
}


void OCamlWatch::debuggerStarted( bool b )
{
    setEnabled( b );
}

