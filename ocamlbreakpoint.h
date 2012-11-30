#ifndef OCAMLBREAKPOINT_H
#define OCAMLBREAKPOINT_H

#include <QString>
#include <QMap>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QTreeWidget>
#include <QCompleter>
#include "breakpoint.h"
#include "debuggercommand.h"

class OCamlBreakpoint : public QWidget
{
    Q_OBJECT

public:
    OCamlBreakpoint( QWidget * parent_p );
    virtual ~OCamlBreakpoint( );

signals:
    bool debugger( const DebuggerCommand & ) ;
public slots:
    void updateBreakpoints();
    void stopDebugging( const QString &, int , int , bool) ;
    void debuggerStarted( bool );
    void  breakPointList( const BreakPoints & );
    void  breakPointHit( const QList<int> & );
protected slots:
    void expressionClicked( QTreeWidgetItem * , int );
protected:
    void closeEvent(QCloseEvent *event);

private:
    QList<int> _breakpoint_hit ;
    BreakPoints _breakpoints;
    void clearData();
    QVBoxLayout *layout_p;
    QTreeWidget *breakpoints_p;
};

#endif

