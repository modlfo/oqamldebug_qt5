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
#include "arguments.h"

class OCamlRun : public QPlainTextEdit
{
    Q_OBJECT

public:
    OCamlRun( QWidget * parent_p, const Arguments & arguments);
    virtual ~OCamlRun( );
    void setArguments(const Arguments &arguments);
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
    void startProcess( int port );
    void clear();
    void terminate();
    void readChannel();
    void appendText(const QByteArray &, const QColor &color = Qt::black );
    QProcess *process_p;
    QTextStream _outstream;
    Arguments _arguments;
    int _cursor_position;
};

#endif
