#ifndef OCAMLWATCH_H
#define OCAMLWATCH_H

#include <QString>
#include <QMap>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QTreeWidget>
#include <QCompleter>
#include "ocamldebug.h"

class OCamlWatch : public QWidget
{
    Q_OBJECT

private:
        struct Watch
        {
            QString variable;
            QString all_output;
            QString value_only;
            bool display;
            bool uptodate;
        };
public:
    OCamlWatch( QWidget * parent_p, int  );
    virtual ~OCamlWatch( );
    const int id ;

signals:
    bool debugger( const DebuggerCommand & ) ;
public slots:
    void updateWatches();
    void watch( const QString & v, bool display );
    void stopDebugging( const QString &, int , int , bool) ;
    void  debuggerCommand( const QString &, const QString &);
    void debuggerStarted(bool b);
protected slots:
    void columnResized( int logical_index, int old_size, int new_size );
    void addNewValue();
    void expressionClicked( QTreeWidgetItem * , int );
protected:
    void closeEvent(QCloseEvent *event);

private:
    QList<Watch> _watches ;
    QString command (const Watch & ) const;
    void clearData();
    QStringList  variables() const;
    void saveWatches();
    void restoreWatches();
    QVBoxLayout *layout_p;
    QHBoxLayout *layout_add_value_p;
    QLabel *add_value_label_p;
    QLineEdit *add_value_p;
    QStringList add_values ;
    QCompleter *add_value_completer_p;
    QTreeWidget *variables_p;
    QRegExp variableRx;
};

#endif

