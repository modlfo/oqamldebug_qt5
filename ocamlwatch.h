#ifndef OCAMLWATCH_H
#define OCAMLWATCH_H

#include <QProcess>
#include <QString>
#include <QMap>
#include <QStringList>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>
#include "ocamlsourcehighlighter.h"
#include "filesystemwatcher.h"

class OCamlWatch : public QWidget
{
    Q_OBJECT

private:
        struct Watch
        {
            QString variable;
            QString value;
            bool display;
        };
public:
    OCamlWatch( QWidget * parent_p, int  );
    virtual ~OCamlWatch( );
    const int id ;

signals:
    bool debugger( const QString &, bool ) ;
public slots:
    void updateWatches();
    void watch( const QString & v, bool display );
    void stopDebugging( const QString &, int , int , bool) ;
    void  debuggerCommand( const QString &, const QString &);
protected:
    void closeEvent(QCloseEvent *event);

private:
    QList<Watch> _watches ;
    QString command (const Watch & ) const;
    void clearData();
    OCamlSourceHighlighter *highlighter;
    QStringList  variables() const;
    void saveWatches();
    void restoreWatches();
    QVBoxLayout *layout_p;
    QTextEdit *editor_p;
};

#endif

