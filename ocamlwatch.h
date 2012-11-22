#ifndef OCAMLWATCH_H
#define OCAMLWATCH_H

#include <QString>
#include <QMap>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <QTreeWidget>

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
protected slots:
    void columnResized( int logical_index, int old_size, int new_size );
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
    QTreeWidget *variables_p;
    QRegExp variableRx;
};

#endif
