#ifndef DEBUGGER_COMMAND_H
#define DEBUGGER_COMMAND_H
#include <QString>

class DebuggerCommand
{
    public:
        enum Option
        {
            IMMEDIATE_COMMAND,
            HIDE_DEBUGGER_OUTPUT,
            HIDE_DEBUGGER_OUTPUT_SHOW_PROMT,
            SHOW_ALL_OUTPUT,
            HIDE_ALL_OUTPUT,
        };
        DebuggerCommand( const QString &command, Option o ) :
            _option( o ),
            _command( command )
        {
        }

        const Option &option() const { return _option ; }
        const QString &command() const { return _command ; }
        const QString &result() const { return _result; }
        void appendResult( const QString &s ) { _result += s; }
        void setOption( Option o ) { _option = o ; }
    private:
        Option _option;
        QString _command;
        QString _result;
};

#endif
