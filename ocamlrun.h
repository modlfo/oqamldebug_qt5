#ifndef OCAMLRUN_H
#define OCAMLRUN_H

#include <QPlainTextEdit>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include "filesystemwatcher.h"
#include "breakpoint.h"
#include "debuggercommand.h"

class OCamlRun : public QPlainTextEdit
{
    Q_OBJECT

public:
    OCamlRun( QWidget * parent_p, const QString &ocamlrun, const QString&app, const QStringList &);
    virtual ~OCamlRun( );
    void setApplication(const QString &app, const QStringList &);
    void setOCamlRun(const QString &);
    void startApplication(int port);

protected:
    void closeEvent(QCloseEvent *event);

    virtual void keyPressEvent ( QKeyEvent * e );
    virtual void keyReleaseEvent ( QKeyEvent * e );

public slots:
    void debuggerStarted( bool );

private slots:
    void receiveDataFromProcessStdOutput();
    void receiveDataFromProcessStdError();

private:
    void startProcess( int port, const QString &program , const QStringList &arguments );
    void clear();
    void terminate();
    void readChannel();
    void appendText(const QByteArray &);
    QProcess *process_p;
    QTextStream _outstream;
    QString _ocamlrun;
    QString _ocamlapp;
    QStringList _arguments;
    int _cursor_position;
};

#endif
