#include "arguments.h"
#include <QFileInfo>
#include <QtDebug>

Arguments::Arguments( const QStringList &a ) : _arguments( a ) 
{
    calcArguments();
}

Arguments::Arguments( const QString &a ) 
{
    _arguments.clear();
    QString current_arg;
    bool between_arguments = false;
    bool quoted_argument = false;
    bool escape = false;
    for( QString::const_iterator itChar = a.begin(); itChar != a.end(); ++itChar )
    {
        if ( between_arguments )
        {
            switch ( itChar->toAscii() )
            {
                case '"':
                    quoted_argument = true;
                    between_arguments = false;
                    current_arg.clear();
                    break;
                case ' ':
                case '\r':
                case '\n':
                case '\t':
                    break;
                default:
                    quoted_argument = false;
                    between_arguments = false;
                    current_arg = *itChar;
                    break;
            }
        }
        else
        {
            if ( escape )
            {
                escape = false;
                current_arg += *itChar;
            }
            else
            {
                switch ( itChar->toAscii() )
                {
                    case '"':
                        quoted_argument = !quoted_argument;
                        break;
                    case ' ':
                    case '\r':
                    case '\n':
                    case '\t':
                        if ( quoted_argument )
                            current_arg += *itChar;
                        else
                        {
                            _arguments << current_arg;
                            between_arguments = true;
                        }
                        break;
                    case '\\':
                        escape = true ;
                        break;
                    default:
                        current_arg += *itChar;
                        break;
                }

            }
        }
    }
    if ( !between_arguments )
        _arguments << current_arg;
    calcArguments();
}

void Arguments::calcArguments()
{
    _ocamlapp.clear();
    _ocamldebug_arguments.clear();
    _app_arguments.clear();

    _ocamldebug_arguments << "-emacs" ;

    bool executable_found             = false;
    foreach ( QString a , _arguments )
    {
        QFileInfo arg_info(a);
        if (  ( ! executable_found )
                && ( ! arg_info.isDir())
                && arg_info.exists()
                && arg_info.isExecutable()
           )
        {
            _ocamlapp = arg_info.absoluteFilePath();
            executable_found = true ;
        }
        else if ( executable_found )
            _app_arguments << a ;
        else
            _ocamldebug_arguments << a ;
    }
}

QString Arguments::toString() const
{
    bool first = true ;
    QString str;
    for ( QStringList::const_iterator itArgs = all().begin(); itArgs != all().end(); ++itArgs )
    {
        if ( !first )
            str += " ";
        if ( itArgs->contains( "\"" ) || itArgs->isEmpty() )
        {
            QString s = *itArgs;
            s.replace( "\\", "\\\\" );
            s.replace( "\"", "\\\"" );
            str += "\"";
            str += s ;
            str += "\"";
        }
        else
            str += *itArgs;
        first = false;
    }

    return str;
}
