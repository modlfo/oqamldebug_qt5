#ifndef OCAMLSTACK_H
#define OCAMLSTACK_H

#include <QString>
#include <QMap>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QTreeWidget>
#include <QCompleter>
#include "ocamldebug.h"

class OCamlStack : public QWidget
{
    Q_OBJECT

private:
        struct StackItem
        {
            int frame;
            int pc;
            QString module;
            int position;
        };
        typedef QList<StackItem> Stack ;
public:
    OCamlStack( QWidget * parent_p );
    virtual ~OCamlStack( );

signals:
    bool debugger( const DebuggerCommand & ) ;
public slots:
    void updateStack();
    void debuggerStarted(bool b);
    void stopDebugging( const QString &, int , int , bool) ;
    void  debuggerCommand( const QString &, const QString &);
protected slots:
    void expressionClicked( QTreeWidgetItem * , int );
protected:
    void closeEvent(QCloseEvent *event);

private:
    Stack _stack ;
    int _start_char, _end_char;
    bool _after;
    QString _file;
    int _current_frame ;
    void clearData();
    QVBoxLayout *layout_p;
    QTreeWidget *stack_p;
    QRegExp stackRx;
};

#endif

