#include "arguments.h"
#include <QFileInfo>

Arguments::Arguments( const QStringList &a ) : _arguments( a ) 
{
    calcArguments();
}

Arguments::Arguments( const QString &a ) 
{
    _arguments = a.split(" ", QString::SkipEmptyParts);
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
    return all().join( " " );
}
